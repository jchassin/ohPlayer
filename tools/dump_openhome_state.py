#!/usr/bin/env python3
"""Dump and diff the OpenHome/UPnP state of renderers (e.g. ohPlayer vs Linn Majik).

Standard library only: SSDP discovery, device.xml/SCPD parsing, SOAP action
calls, GENA SUBSCRIBE with initial-event capture. No pip install needed.

Usage:
  # Dump explicit device.xml locations (same style as mydemo.py):
  python3 tools/dump_openhome_state.py dump \
      --location http://192.168.10.39:55178/<udn>/Upnp/device.xml \
      --location http://192.168.10.30:51000/<udn>/Upnp/device.xml \
      --out dumps/

  # Or discover renderers on the LAN via SSDP:
  python3 tools/dump_openhome_state.py dump --discover --out dumps/

  # Compare two dumps (what differs at init between ohPlayer and Majik):
  python3 tools/dump_openhome_state.py diff dumps/<ohplayer>.json dumps/<majik>.json

  # Reproduce a radio start and watch state transitions (Buffering...):
  python3 tools/dump_openhome_state.py dump --location http://<ohplayer>/device.xml \
      --observe 20 --set-channel https://icecast.radiofrance.fr/franceinter-hifi.aac --play
"""

import argparse
import http.client
import json
import socket
import sys
import threading
import time
import urllib.error
import urllib.parse
import urllib.request
import xml.etree.ElementTree as ET
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from xml.sax.saxutils import escape

OH_SOURCE_ST = "urn:av-openhome-org:device:Source"
MR_ST = "urn:schemas-upnp-org:device:MediaRenderer"

SAFE_QUERY_ACTIONS = {
    "Product", "Room", "Standby", "StandbyTransitioning",
    "SourceIndex", "Source", "SourceXml", "Attributes",
    "Volume", "Mute", "Balance", "Fade", "Loudness",
    "TransportState", "StreamInfo", "ModeInfo",
    "Track", "Metadata", "Details", "State", "TrackUri",
    "Channel", "Id", "IdArray", "ReadList", "Preset", "Presets",
    "Status", "Audio", "Sender", "SoftwareStatus", "PlayMode",
    "CurrentValidPlayModes", "SupportedVersions",
}

VOLATILE_KEYS = {"dumped_at", "location", "base_url", "ssdp", "sid", "timeout",
                 "SEQ", "SID", "raw_lastchange", "label", "subscribe_error"}

RADIO_SERVICE_ID = "urn:av-openhome-org:serviceId:Radio"


def local(tag):
    return tag.rsplit("}", 1)[-1]


def child(elem, name):
    for c in elem:
        if local(c.tag) == name:
            return c
    return None


def children(elem, name):
    return [c for c in elem if local(c.tag) == name]


def text_of(elem, name):
    c = child(elem, name)
    if c is None or c.text is None:
        return ""
    return c.text.strip()


def fetch(url, timeout=10):
    req = urllib.request.Request(url, headers={"User-Agent": "ohstate-dump/1.0"})
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return resp.read()


def ssdp_search(st, timeout=3):
    results = []
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.settimeout(timeout)
    msg = (
        "M-SEARCH * HTTP/1.1\r\n"
        "HOST: 239.255.255.250:1900\r\n"
        'MAN: "ssdp:discover"\r\n'
        "MX: 2\r\n"
        f"ST: {st}\r\n\r\n"
    ).encode()
    try:
        sock.sendto(msg, ("239.255.255.250", 1900))
        deadline = time.time() + timeout
        while time.time() < deadline:
            try:
                data, _ = sock.recvfrom(65535)
            except socket.timeout:
                break
            headers = {}
            for line in data.decode(errors="replace").split("\r\n")[1:]:
                if ":" in line:
                    k, v = line.split(":", 1)
                    headers[k.strip().upper()] = v.strip()
            if "LOCATION" in headers:
                results.append({
                    "st": headers.get("ST", st),
                    "location": headers["LOCATION"],
                    "usn": headers.get("USN", ""),
                    "server": headers.get("SERVER", ""),
                })
    finally:
        sock.close()
    return results


def parse_device(dev_elem, base_url):
    services = []
    sl = child(dev_elem, "serviceList")
    if sl is not None:
        for s in children(sl, "service"):
            services.append({
                "serviceType": text_of(s, "serviceType"),
                "serviceId": text_of(s, "serviceId"),
                "SCPDURL": urllib.parse.urljoin(base_url, text_of(s, "SCPDURL")),
                "controlURL": urllib.parse.urljoin(base_url, text_of(s, "controlURL")),
                "eventSubURL": urllib.parse.urljoin(base_url, text_of(s, "eventSubURL")),
            })
    embedded = []
    dl = child(dev_elem, "deviceList")
    if dl is not None:
        for d in children(dl, "device"):
            embedded.append(parse_device(d, base_url))
    return {
        "deviceType": text_of(dev_elem, "deviceType"),
        "UDN": text_of(dev_elem, "UDN"),
        "friendlyName": text_of(dev_elem, "friendlyName"),
        "manufacturer": text_of(dev_elem, "manufacturer"),
        "modelName": text_of(dev_elem, "modelName"),
        "presentationURL": urllib.parse.urljoin(base_url, text_of(dev_elem, "presentationURL")),
        "services": services,
        "embeddedDevices": embedded,
    }


def parse_scpd(xml_bytes):
    root = ET.fromstring(xml_bytes)
    actions = {}
    al = child(root, "actionList")
    if al is not None:
        for a in children(al, "action"):
            name = text_of(a, "name")
            args = []
            argl = child(a, "argumentList")
            if argl is not None:
                for ar in children(argl, "argument"):
                    args.append({
                        "name": text_of(ar, "name"),
                        "direction": text_of(ar, "direction"),
                    })
            actions[name] = args
    state_vars = []
    svt = child(root, "serviceStateTable")
    if svt is not None:
        for sv in children(svt, "stateVariable"):
            state_vars.append({
                "name": text_of(sv, "name"),
                "dataType": text_of(sv, "dataType"),
                "sendEvents": sv.get("sendEvents", ""),
            })
    return {"actions": actions, "stateVariables": state_vars}


SOAP_TEMPLATE = (
    '<?xml version="1.0" encoding="utf-8"?>'
    '<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/"'
    ' s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">'
    "<s:Body>"
    '<u:{action} xmlns:u="{service_type}">'
    "{args}"
    "</u:{action}>"
    "</s:Body></s:Envelope>"
)


def soap_call(control_url, service_type, action, args=None):
    args = args or {}
    arg_xml = "".join(
        f"<{k}>{escape(str(v))}</{k}>" for k, v in args.items()
    )
    body = SOAP_TEMPLATE.format(action=action, service_type=service_type, args=arg_xml).encode()
    req = urllib.request.Request(
        control_url,
        data=body,
        headers={
            "Content-Type": 'text/xml; charset="utf-8"',
            "SOAPACTION": f'"{service_type}#{action}"',
        },
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            data = resp.read()
            status = resp.status
    except urllib.error.HTTPError as e:
        data = e.read()
        status = e.code
    result = {"http_status": status}
    try:
        root = ET.fromstring(data)
    except ET.ParseError:
        result["error"] = data.decode(errors="replace")[:500]
        return result
    body_el = child(root, "Body")
    if body_el is None:
        result["error"] = "no Body in response"
        return result
    first = None
    for c in body_el:
        first = c
        break
    if first is None:
        result["error"] = "empty Body"
        return result
    if local(first.tag) == "Fault":
        result["fault"] = text_of(first, "faultstring")
        return result
    out = {}
    for c in first:
        out[local(c.tag)] = c.text or ""
    result["out"] = out
    return result


class _NotifyHandler(BaseHTTPRequestHandler):
    def do_NOTIFY(self):
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length) if length else b""
        sid = self.headers.get("SID", "")
        seq = self.headers.get("SEQ", "")
        self.server.events.append({"sid": sid, "seq": seq, "body": body, "at": time.time()})
        if sid not in self.server.first_by_sid:
            self.server.first_by_sid[sid] = body
        self.send_response(200)
        self.send_header("Content-Length", "0")
        self.end_headers()

    def log_message(self, *args):
        pass


class NotifyServer(ThreadingHTTPServer):
    def __init__(self, addr):
        super().__init__(addr, _NotifyHandler)
        self.events = []
        self.first_by_sid = {}


def local_ip_for(remote_host):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect((remote_host, 80))
        return s.getsockname()[0]
    except OSError:
        return "127.0.0.1"
    finally:
        s.close()


def http_subscribe(event_url, callback_url, timeout=1800):
    u = urllib.parse.urlparse(event_url)
    port = u.port or 80
    conn = http.client.HTTPConnection(u.hostname, port, timeout=10)
    path = u.path + (f"?{u.query}" if u.query else "")
    conn.request(
        "SUBSCRIBE",
        path,
        headers={
            "HOST": f"{u.hostname}:{port}",
            "CALLBACK": f"<{callback_url}>",
            "NT": "upnp:event",
            "TIMEOUT": f"Second-{timeout}",
        },
    )
    resp = conn.getresponse()
    resp.read()
    sid = resp.getheader("SID")
    to = resp.getheader("TIMEOUT")
    conn.close()
    if resp.status != 200 or not sid:
        raise RuntimeError(f"SUBSCRIBE failed: HTTP {resp.status}")
    return sid, to


def http_unsubscribe(event_url, sid):
    u = urllib.parse.urlparse(event_url)
    port = u.port or 80
    conn = http.client.HTTPConnection(u.hostname, port, timeout=10)
    path = u.path + (f"?{u.query}" if u.query else "")
    conn.request(
        "UNSUBSCRIBE",
        path,
        headers={"HOST": f"{u.hostname}:{port}", "SID": sid},
    )
    resp = conn.getresponse()
    resp.read()
    conn.close()


def parse_lastchange(xml_text):
    out = {}
    try:
        root = ET.fromstring(xml_text)
    except ET.ParseError:
        return {"_raw": xml_text}
    for elem in root.iter():
        if local(elem.tag) == "Event":
            continue
        if "val" in elem.attrib:
            out[local(elem.tag)] = elem.attrib["val"]
    return out


def parse_propertyset(xml_bytes):
    root = ET.fromstring(xml_bytes)
    out = {}
    for prop in root:
        if local(prop.tag) != "property":
            continue
        for var in prop:
            name = local(var.tag)
            value = var.text or ""
            if name == "LastChange":
                out["LastChange"] = parse_lastchange(value)
            else:
                out[name] = value
    return out


def didl_for_uri(uri, title=""):
    return (
        '<DIDL-Lite xmlns:dc="http://purl.org/dc/elements/1.1/"'
        ' xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/"'
        ' xmlns="urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/">'
        '<item id="" parentID="" restricted="True">'
        f"<dc:title>{escape(title)}</dc:title>"
        f'<res protocolInfo="*:*:*:*">{escape(uri)}</res>'
        "<upnp:class>object.item.audioItem</upnp:class>"
        "</item></DIDL-Lite>"
    )


def flatten_devices(dev, parent_type=""):
    out = [dev]
    for e in dev.get("embeddedDevices", []):
        out.extend(flatten_devices(e, dev.get("deviceType", "")))
    return out


def is_safe_query(name, in_args):
    return not in_args and (name.startswith("Get") or name in SAFE_QUERY_ACTIONS)


def dump_location(location, observe=0, set_channel=None, play=False, label=None):
    xml_bytes = fetch(location)
    root = ET.fromstring(xml_bytes)
    dev_elem = child(root, "device")
    if dev_elem is None:
        raise RuntimeError(f"no <device> in {location}")
    parsed = parse_device(dev_elem, location)

    host = urllib.parse.urlparse(location).hostname
    my_ip = local_ip_for(host)
    server = NotifyServer((my_ip, 0))
    cb_port = server.server_address[1]
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    callback_url = f"http://{my_ip}:{cb_port}/notify"

    sids = {}
    try:
        for dev in flatten_devices(parsed):
            for svc in dev["services"]:
                try:
                    sid, to = http_subscribe(svc["eventSubURL"], callback_url)
                    sids[sid] = (svc, to)
                except Exception as e:
                    svc["subscribe_error"] = str(e)

        deadline = time.time() + 6
        while time.time() < deadline and len(server.first_by_sid) < len(sids):
            time.sleep(0.2)
        time.sleep(0.5)

        if set_channel:
            for dev in flatten_devices(parsed):
                for svc in dev["services"]:
                    if svc["serviceId"] == RADIO_SERVICE_ID:
                        r = soap_call(svc["controlURL"], svc["serviceType"], "SetChannel",
                                      {"Uri": set_channel, "Metadata": didl_for_uri(set_channel)})
                        print(f"[set-channel] -> {json.dumps(r)[:200]}")
        if play:
            for dev in flatten_devices(parsed):
                for svc in dev["services"]:
                    if svc["serviceId"] == RADIO_SERVICE_ID:
                        r = soap_call(svc["controlURL"], svc["serviceType"], "Play")
                        print(f"[play] -> {json.dumps(r)[:200]}")

        t0 = time.time()
        if observe > 0:
            print(f"[observe] watching {observe}s ...")
            seen = 0
            while time.time() - t0 < observe:
                time.sleep(0.2)
                while seen < len(server.events):
                    ev = server.events[seen]
                    seen += 1
                    dt = ev["at"] - t0
                    try:
                        state = parse_propertyset(ev["body"])
                    except ET.ParseError:
                        continue
                    svc = sids.get(ev["sid"], (None, None))[0]
                    sid_name = svc["serviceId"].split(":")[-1] if svc else ev["sid"]
                    for k, v in state.items():
                        if k == "LastChange" and isinstance(v, dict):
                            for kk, vv in v.items():
                                print(f"[{dt:7.2f}s] {sid_name}.LastChange.{kk} = {vv}")
                        else:
                            print(f"[{dt:7.2f}s] {sid_name}.{k} = {str(v)[:160]}")

        doc = {
            "label": label or parsed.get("friendlyName") or parsed.get("UDN"),
            "location": location,
            "base_url": location,
            "dumped_at": datetime.now(timezone.utc).isoformat(),
            "rootDevice": parsed,
            "services": {},
        }
        for dev in flatten_devices(parsed):
            dev_key = dev["deviceType"] + " / " + dev["UDN"]
            for svc in dev["services"]:
                entry = {
                    "serviceType": svc["serviceType"],
                    "serviceId": svc["serviceId"],
                    "controlURL": svc["controlURL"],
                    "eventSubURL": svc["eventSubURL"],
                }
                try:
                    scpd = parse_scpd(fetch(svc["SCPDURL"]))
                    entry["scpd"] = {
                        "actions": {n: [a["name"] for a in args if a["direction"] == "in"]
                                    for n, args in scpd["actions"].items()},
                        "stateVariables": [s["name"] for s in scpd["stateVariables"]],
                    }
                except Exception as e:
                    entry["scpd_error"] = str(e)
                sid = None
                for s, (sv, _to) in sids.items():
                    if sv is svc:
                        sid = s
                        break
                if sid is not None:
                    entry["sid"] = sid
                    if sid in server.first_by_sid:
                        try:
                            entry["initial_event"] = parse_propertyset(server.first_by_sid[sid])
                        except ET.ParseError as e:
                            entry["initial_event_error"] = str(e)
                if "scpd" in entry:
                    queries = {}
                    for name, in_args in entry["scpd"]["actions"].items():
                        if is_safe_query(name, in_args):
                            r = soap_call(svc["controlURL"], svc["serviceType"], name)
                            queries[name] = r
                    entry["query_results"] = queries
                key = dev["deviceType"] + " :: " + svc["serviceId"]
                doc["services"][key] = entry
    finally:
        for sid, (svc, _to) in sids.items():
            try:
                http_unsubscribe(svc["eventSubURL"], sid)
            except Exception:
                pass
        server.shutdown()
        server.server_close()
    return doc


def short(v, n=220):
    s = json.dumps(v, ensure_ascii=False, sort_keys=True, default=str)
    return s if len(s) <= n else s[:n] + "..."


def norm(v):
    if isinstance(v, str) and v.startswith("http"):
        return urllib.parse.urlparse(v).path
    if isinstance(v, dict):
        return {k: norm(x) for k, x in v.items()}
    if isinstance(v, list):
        return [norm(x) for x in v]
    return v


def diff(a, b, path="", out=None):
    if out is None:
        out = []
    a = norm(a)
    b = norm(b)
    if isinstance(a, dict) and isinstance(b, dict):
        for k in sorted(set(a) | set(b)):
            if k in VOLATILE_KEYS:
                continue
            p = f"{path}/{k}"
            if k not in a:
                out.append(f"{p}: only in B = {short(b[k])}")
            elif k not in b:
                out.append(f"{p}: only in A = {short(a[k])}")
            else:
                diff(a[k], b[k], p, out)
    elif isinstance(a, list) and isinstance(b, list):
        if a != b:
            out.append(f"{path}: A={short(a)} vs B={short(b)}")
    else:
        if a != b:
            out.append(f"{path}: A={short(a)} vs B={short(b)}")
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    d = sub.add_parser("dump", help="dump renderer state(s) to JSON")
    d.add_argument("--location", action="append", default=[],
                   help="device.xml URL (repeatable)")
    d.add_argument("--discover", action="store_true", help="SSDP search on the LAN")
    d.add_argument("--search-timeout", type=float, default=3.0)
    d.add_argument("--out", default=None,
                   help="output file (single device) or directory (multiple)")
    d.add_argument("--observe", type=float, default=0,
                   help="seconds to watch GENA events after subscribing")
    d.add_argument("--set-channel", default=None, help="Radio.SetChannel Uri to send")
    d.add_argument("--play", action="store_true", help="Radio.Play after SetChannel")

    c = sub.add_parser("diff", help="diff two dump JSON files")
    c.add_argument("a")
    c.add_argument("b")
    c.add_argument("--label-a", default="A")
    c.add_argument("--label-b", default="B")

    args = ap.parse_args()

    if args.cmd == "diff":
        with open(args.a) as f:
            da = json.load(f)
        with open(args.b) as f:
            db = json.load(f)
        lines = diff(da, db)
        print(f"--- {args.label_a}: {da.get('label')} ({da.get('location')})")
        print(f"+++ {args.label_b}: {db.get('label')} ({db.get('location')})")
        if not lines:
            print("no differences")
        for ln in lines:
            print(ln)
        return 0

    locations = list(args.location)
    if args.discover:
        seen = set()
        for st in (OH_SOURCE_ST, MR_ST):
            for r in ssdp_search(st, args.search_timeout):
                if r["location"] not in seen:
                    seen.add(r["location"])
                    locations.append(r["location"])
                    print(f"[discover] {r['st']} -> {r['location']} ({r['server']})")
    if not locations:
        ap.error("no --location given and --discover found nothing")

    docs = []
    for i, loc in enumerate(locations):
        print(f"[dump] {loc}")
        doc = dump_location(loc, observe=args.observe,
                            set_channel=args.set_channel, play=args.play,
                            label=f"device{i}")
        docs.append(doc)

    if args.out:
        import os
        if len(docs) == 1 and not args.out.endswith(os.sep):
            with open(args.out, "w") as f:
                json.dump(docs[0], f, indent=2, sort_keys=True, ensure_ascii=False)
            print(f"[out] {args.out}")
        else:
            os.makedirs(args.out, exist_ok=True)
            for doc in docs:
                name = (doc.get("label") or "device").replace("/", "_")
                udn = ""
                rd = doc.get("rootDevice", {})
                udn = rd.get("UDN", "").replace("uuid:", "")[:12]
                path = os.path.join(args.out, f"{name}-{udn}.json")
                with open(path, "w") as f:
                    json.dump(doc, f, indent=2, sort_keys=True, ensure_ascii=False)
                print(f"[out] {path}")
    else:
        if len(docs) == 1:
            print(json.dumps(docs[0], indent=2, sort_keys=True, ensure_ascii=False))
        else:
            for doc in docs:
                print(f"=== {doc['label']} {doc['location']}")
                print(json.dumps(doc, indent=2, sort_keys=True, ensure_ascii=False))
    if len(docs) >= 2:
        print("\n===== DIFF (first two devices) =====")
        for ln in diff(docs[0], docs[1]):
            print(ln)
    return 0


if __name__ == "__main__":
    sys.exit(main())

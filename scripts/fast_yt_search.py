#!/usr/bin/env python3
"""
fast_yt_search.py — Mousiki Aidil Edition
YouTube Innertube search. No API key. No OAuth.

Usage:
  fast_yt_search.py <query> [limit]
  fast_yt_search.py stream <video_id>
  fast_yt_search.py trending [limit]

Output: one JSON line per result: {"id":"...","title":"...","uploader":"...","duration":225}
"""

import json, sys, re, urllib.request, urllib.error

UA = "Mozilla/5.0 (X11; Linux x86_64; rv:120.0) Gecko/20100101 Firefox/120.0"
INNERTUBE_URL = "https://www.youtube.com/youtubei/v1/search"
INNERTUBE_PAYLOAD = {
    "context": {
        "client": {
            "clientName": "WEB",
            "clientVersion": "2.20231219.01.00",
            "hl": "en",
            "gl": "US"
        }
    }
}

def parse_duration(text):
    """'3:45' or '1:02:03' -> seconds int"""
    if not text:
        return 0
    parts = text.strip().split(":")
    try:
        if len(parts) == 2:
            return int(parts[0]) * 60 + int(parts[1])
        elif len(parts) == 3:
            return int(parts[0]) * 3600 + int(parts[1]) * 60 + int(parts[2])
    except Exception:
        pass
    return 0

def innertube_search(query, limit=10):
    payload = dict(INNERTUBE_PAYLOAD)
    payload["query"] = query
    data = json.dumps(payload).encode()
    req = urllib.request.Request(
        INNERTUBE_URL,
        data=data,
        headers={"Content-Type": "application/json", "User-Agent": UA}
    )
    try:
        with urllib.request.urlopen(req, timeout=6) as resp:
            raw = json.loads(resp.read().decode("utf-8", errors="replace"))
    except Exception:
        return []

    results = []
    # Walk the response tree to get video items
    try:
        contents = (raw.get("contents", {})
                       .get("twoColumnSearchResultsRenderer", {})
                       .get("primaryContents", {})
                       .get("sectionListRenderer", {})
                       .get("contents", []))
        for section in contents:
            items = section.get("itemSectionRenderer", {}).get("contents", [])
            for item in items:
                v = item.get("videoRenderer")
                if not v:
                    continue
                vid_id = v.get("videoId", "")
                title = ""
                try:
                    title = v["title"]["runs"][0]["text"]
                except Exception:
                    pass
                uploader = ""
                try:
                    uploader = v["ownerText"]["runs"][0]["text"]
                except Exception:
                    pass
                dur_text = ""
                try:
                    dur_text = v["lengthText"]["simpleText"]
                except Exception:
                    pass
                duration = parse_duration(dur_text)
                if vid_id and title:
                    results.append({
                        "id": vid_id,
                        "title": title,
                        "uploader": uploader,
                        "duration": duration
                    })
                if len(results) >= limit:
                    break
            if len(results) >= limit:
                break
    except Exception:
        pass
    return results

def do_stream(video_id):
    """Print the YouTube watch URL for yt-dlp to consume."""
    print(f"https://www.youtube.com/watch?v={video_id}")

def do_trending(limit=10):
    # Search for trending music as a proxy (Innertube trending requires different endpoint)
    return innertube_search("top music trending 2024", limit)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(0)

    cmd = sys.argv[1]

    if cmd == "stream":
        if len(sys.argv) >= 3:
            do_stream(sys.argv[2])
        sys.exit(0)

    if cmd == "trending":
        limit = int(sys.argv[2]) if len(sys.argv) > 2 else 10
        for r in do_trending(limit):
            print(json.dumps(r))
        sys.exit(0)

    # Normal search
    query = cmd
    limit = int(sys.argv[2]) if len(sys.argv) > 2 else 10
    for r in innertube_search(query, limit):
        print(json.dumps(r))

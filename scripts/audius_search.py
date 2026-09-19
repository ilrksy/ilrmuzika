#!/usr/bin/env python3
"""
audius_search.py — Mousiki Aidil Edition
Audius public API. No API key. No OAuth. Free streaming like Spotify.

Usage:
  audius_search.py <query> [limit]
  audius_search.py trending [limit]
  audius_search.py stream <track_id>

Output: one JSON line per result: {"id":"...","title":"...","uploader":"...","duration_sec":225}
For 'stream': prints the direct CDN stream URL.
"""

import json, sys, urllib.request, urllib.parse, urllib.error

UA = "ilrmuzika/1.0 (https://github.com/aidil/ilrmuzika)"
AUDIUS_BASE = "https://api.audius.co"

def api_get(path, params=None):
    url = AUDIUS_BASE + path
    if params:
        url += "?" + urllib.parse.urlencode(params)
    req = urllib.request.Request(url, headers={"User-Agent": UA, "Accept": "application/json"})
    try:
        with urllib.request.urlopen(req, timeout=8) as resp:
            return json.loads(resp.read().decode("utf-8", errors="replace"))
    except Exception:
        return None

def parse_tracks(data, limit):
    results = []
    if not data:
        return results
    tracks = data.get("data", [])
    for t in tracks[:limit]:
        track_id = t.get("id", "")
        title = t.get("title", "")
        uploader = ""
        user = t.get("user", {})
        if user:
            uploader = user.get("name", user.get("handle", ""))
        duration = t.get("duration", 0)
        if track_id and title:
            results.append({
                "id": track_id,
                "title": title,
                "uploader": uploader,
                "duration_sec": duration
            })
    return results

def do_search(query, limit=10):
    data = api_get("/v1/tracks/search", {"query": query, "limit": limit})
    return parse_tracks(data, limit)

def do_trending(limit=10):
    data = api_get("/v1/tracks/trending", {"limit": limit})
    return parse_tracks(data, limit)

def do_stream(track_id):
    """
    GET /v1/tracks/{id}/stream -> 302 redirect to CDN URL.
    We follow the redirect and print the final URL.
    """
    url = f"{AUDIUS_BASE}/v1/tracks/{track_id}/stream"
    req = urllib.request.Request(url, headers={"User-Agent": UA})
    try:
        # Don't follow redirects automatically — extract Location header
        opener = urllib.request.build_opener(urllib.request.HTTPRedirectHandler())
        # Actually just follow and get final URL
        with urllib.request.urlopen(req, timeout=10) as resp:
            print(resp.url)
    except urllib.error.HTTPError as e:
        if e.code in (301, 302, 303, 307, 308):
            loc = e.headers.get("Location", "")
            if loc:
                print(loc)
    except Exception:
        pass

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
    for r in do_search(query, limit):
        print(json.dumps(r))

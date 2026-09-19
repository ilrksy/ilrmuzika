#!/usr/bin/env python3
"""
mb_lookup.py — Mousiki Aidil Edition
MusicBrainz metadata enrichment. No API key. No OAuth.

Usage:
  mb_lookup.py <title> [artist]

Output: single JSON line: {"title":"...","artist":"...","album":"...","year":"2023","genre":"...","mbid":"..."}
Silent on error.
"""

import json, sys, urllib.request, urllib.parse, time

UA = "ilrmuzika/1.0 (aidil@ilrmuzika.local)"
BASE = "https://musicbrainz.org/ws/2"

def mb_search(title, artist=""):
    # Build query: 'recording:TITLE AND artist:ARTIST' or just title
    q = f'recording:"{urllib.parse.quote(title)}"'
    if artist:
        q += f' AND artist:"{urllib.parse.quote(artist)}"'

    params = urllib.parse.urlencode({"query": q, "fmt": "json", "limit": "1"})
    url = f"{BASE}/recording?{params}"
    req = urllib.request.Request(url, headers={"User-Agent": UA, "Accept": "application/json"})
    try:
        with urllib.request.urlopen(req, timeout=5) as resp:
            data = json.loads(resp.read().decode("utf-8", errors="replace"))
    except Exception:
        return None

    recs = data.get("recordings", [])
    if not recs:
        return None
    rec = recs[0]

    out_title = rec.get("title", title)
    mbid = rec.get("id", "")
    out_artist = ""
    artist_credits = rec.get("artist-credit", [])
    if artist_credits:
        out_artist = artist_credits[0].get("artist", {}).get("name", "")

    out_album = ""
    out_year = ""
    releases = rec.get("releases", [])
    if releases:
        rel = releases[0]
        out_album = rel.get("title", "")
        date = rel.get("date", "")
        if date:
            out_year = date[:4]

    return {
        "title": out_title,
        "artist": out_artist,
        "album": out_album,
        "year": out_year,
        "genre": "",  # MusicBrainz genres need a separate lookup; skip for speed
        "mbid": mbid
    }

if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(0)
    title = sys.argv[1]
    artist = sys.argv[2] if len(sys.argv) > 2 else ""
    result = mb_search(title, artist)
    if result:
        print(json.dumps(result))

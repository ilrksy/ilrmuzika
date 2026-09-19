#include "online_source.h"
#include "process_util.h"
#include <cctype>
#include <sstream>
#include <filesystem>
#include <unistd.h>

namespace muisc {

namespace fs = std::filesystem;

static fs::path find_fast_search_script() {
    fs::path cwd_candidate = fs::path("scripts") / "fast_yt_search.py";
    if (fs::exists(cwd_candidate)) return cwd_candidate;

    char exe_buf[4096];
    ssize_t n = readlink("/proc/self/exe", exe_buf, sizeof(exe_buf) - 1);
    if (n > 0) {
        exe_buf[n] = '\0';
        fs::path exe_dir = fs::path(exe_buf).parent_path();
        fs::path p = exe_dir / "scripts" / "fast_yt_search.py";
        if (fs::exists(p)) return p;
        p = exe_dir.parent_path() / "scripts" / "fast_yt_search.py";
        if (fs::exists(p)) return p;
    }
    return cwd_candidate;
}

// Same tiny flat-JSON string extractor used in lyrics_fetcher.cpp — kept
// local here since yt-dlp's per-line JSON objects are the only thing that
// needs it in this file, not worth a shared JSON dependency for two spots.
static bool json_get_string(const std::string& json, const std::string& key, std::string& out) {
    std::string needle = "\"" + key + "\"";
    size_t kpos = json.find(needle);
    if (kpos == std::string::npos) return false;
    size_t colon = json.find(':', kpos + needle.size());
    if (colon == std::string::npos) return false;
    size_t qstart = json.find('"', colon);
    if (qstart == std::string::npos) return false;
    size_t i = qstart + 1;
    std::string raw;
    while (i < json.size()) {
        if (json[i] == '\\' && i + 1 < json.size()) { raw += json[i]; raw += json[i + 1]; i += 2; continue; }
        if (json[i] == '"') break;
        raw += json[i];
        ++i;
    }
    // unescape the common cases
    std::string clean;
    for (size_t j = 0; j < raw.size(); ++j) {
        if (raw[j] == '\\' && j + 1 < raw.size()) {
            char n = raw[j + 1];
            if (n == 'n') { clean += ' '; ++j; continue; }
            if (n == '"' || n == '\\' || n == '/') { clean += n; ++j; continue; }
        }
        clean += raw[j];
    }
    out = clean;
    return true;
}

// Bare (unquoted) numeric field, e.g. "duration": 213.0 or "duration": 213.
// yt-dlp's flat-playlist listings include this for most extractors, but
// not all -- false/untouched `out` just means "unknown", not an error.
static bool json_get_number(const std::string& json, const std::string& key, double& out) {
    std::string needle = "\"" + key + "\"";
    size_t kpos = json.find(needle);
    if (kpos == std::string::npos) return false;
    size_t colon = json.find(':', kpos + needle.size());
    if (colon == std::string::npos) return false;
    size_t i = colon + 1;
    while (i < json.size() && (json[i] == ' ' || json[i] == '\t')) ++i;
    if (i >= json.size() || json[i] == 'n') return false; // null / nothing there
    size_t start = i;
    while (i < json.size() && (std::isdigit(static_cast<unsigned char>(json[i])) || json[i] == '-' ||
                                json[i] == '+' || json[i] == '.' || json[i] == 'e' || json[i] == 'E')) ++i;
    if (i == start) return false;
    try {
        out = std::stod(json.substr(start, i - start));
    } catch (...) {
        return false;
    }
    return true;
}

std::vector<OnlineResult> OnlineSource::search(const std::string& query, int count) {
    std::vector<OnlineResult> results;
    if (query.empty()) return results;

    // 1) Fast path: Innertube API via python script (~0.2s, no yt-dlp startup overhead)
    fs::path script = find_fast_search_script();
    if (fs::exists(script)) {
        std::string fast_cmd = "python3 " + shell_quote(script.string()) + " " +
                               shell_quote(query) + " " + std::to_string(count);
        ProcResult fast_r = run_capture(fast_cmd);
        if (!fast_r.out.empty()) {
            std::istringstream stream(fast_r.out);
            std::string line;
            while (std::getline(stream, line)) {
                if (line.empty() || line[0] != '{') continue;
                OnlineResult item;
                std::string uploader;
                json_get_string(line, "id", item.video_id);
                json_get_string(line, "title", item.title);
                if (!json_get_string(line, "uploader", uploader)) {
                    json_get_string(line, "channel", uploader);
                }
                item.uploader = uploader;
                json_get_number(line, "duration", item.duration_sec);
                if (!item.video_id.empty() && !item.title.empty()) results.push_back(std::move(item));
            }
            if (!results.empty()) return results;
        }
    }

    // 2) Fallback: yt-dlp flat-playlist search (relaxed filter: don't restrict categories or drop <90s tracks)
    std::string cmd = "yt-dlp -4 --no-warnings --flat-playlist -j "
                      "\"ytsearch" + std::to_string(count) + ":" + query + "\"";
    ProcResult r = run_capture(cmd);
    if (r.out.empty()) return results;

    std::istringstream stream(r.out);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] != '{') continue;
        OnlineResult item;
        std::string uploader;
        json_get_string(line, "id", item.video_id);
        json_get_string(line, "title", item.title);
        if (!json_get_string(line, "uploader", uploader)) {
            json_get_string(line, "channel", uploader);
        }
        item.uploader = uploader;
        json_get_number(line, "duration", item.duration_sec);
        if (!item.video_id.empty() && !item.title.empty()) results.push_back(std::move(item));
    }
    return results;
}

std::vector<OnlineResult> OnlineSource::list_playlist(const std::string& url, std::string* error_out) {
    std::vector<OnlineResult> results;
    std::string trimmed = url;
    // Trim incidental whitespace a paste often carries.
    while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t')) trimmed.erase(trimmed.begin());
    while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t' || trimmed.back() == '\r' || trimmed.back() == '\n')) trimmed.pop_back();
    if (trimmed.empty()) {
        if (error_out) *error_out = "empty link";
        return results;
    }
    if (trimmed.find("http://") != 0 && trimmed.find("https://") != 0) {
        if (error_out) *error_out = "not a URL -- paste a youtube.com/playlist?list=... link";
        return results;
    }

    // --flat-playlist -j lists every entry (id/title/uploader) without
    // resolving/downloading any of them -- same fast enumeration
    // approach as search() above. Works for a playlist URL (many
    // entries) and degrades gracefully to a single entry for a plain
    // video URL.
    std::string cmd = "yt-dlp -4 --no-warnings --flat-playlist -j " + shell_quote(trimmed);
    ProcResult r = run_capture(cmd);
    if (!r.ok() || r.out.empty()) {
        if (error_out) *error_out = "yt-dlp couldn't list that link (exit " + std::to_string(r.exit_code) + ")";
        return results;
    }

    std::istringstream stream(r.out);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] != '{') continue;
        OnlineResult item;
        std::string uploader;
        json_get_string(line, "id", item.video_id);
        json_get_string(line, "title", item.title);
        if (!json_get_string(line, "uploader", uploader)) {
            json_get_string(line, "channel", uploader);
        }
        item.uploader = uploader;
        json_get_number(line, "duration", item.duration_sec);
        if (!item.video_id.empty()) {
            if (item.title.empty()) item.title = item.video_id;
            results.push_back(std::move(item));
        }
    }
    if (results.empty() && error_out) *error_out = "no videos found in that playlist";
    return results;
}

} // namespace muisc

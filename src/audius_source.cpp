#include "audius_source.h"
#include "process_util.h"
#include <sstream>

namespace muisc {

// Minimal JSON string field extractor (same pattern as online_source.cpp)
static bool json_str(const std::string& json, const std::string& key, std::string& out) {
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
        if (json[i] == '\\' && i + 1 < json.size()) { raw += json[i + 1]; i += 2; continue; }
        if (json[i] == '"') break;
        raw += json[i++];
    }
    out = raw;
    return true;
}

static bool json_num(const std::string& json, const std::string& key, double& out) {
    std::string needle = "\"" + key + "\"";
    size_t kpos = json.find(needle);
    if (kpos == std::string::npos) return false;
    size_t colon = json.find(':', kpos + needle.size());
    if (colon == std::string::npos) return false;
    size_t i = colon + 1;
    while (i < json.size() && (json[i] == ' ' || json[i] == '\t')) ++i;
    if (i >= json.size() || json[i] == 'n') return false;
    size_t start = i;
    while (i < json.size() && (std::isdigit((unsigned char)json[i]) || json[i] == '.' ||
                                json[i] == '-' || json[i] == '+' || json[i] == 'e')) ++i;
    if (i == start) return false;
    try { out = std::stod(json.substr(start, i - start)); } catch (...) { return false; }
    return true;
}

std::vector<AudiusTrack> AudiusSource::parse_output(const std::string& raw, int limit) {
    std::vector<AudiusTrack> results;
    std::istringstream stream(raw);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] != '{') continue;
        AudiusTrack t;
        json_str(line, "id", t.id);
        json_str(line, "title", t.title);
        json_str(line, "uploader", t.uploader);
        double dur = 0.0;
        json_num(line, "duration_sec", dur);
        t.duration_sec = dur;
        if (!t.id.empty() && !t.title.empty()) results.push_back(std::move(t));
        if (static_cast<int>(results.size()) >= limit) break;
    }
    return results;
}

std::vector<AudiusTrack> AudiusSource::search(const std::string& query, int count) {
    std::string script = (scripts_dir_ / "audius_search.py").string();
    std::string cmd = "python3 " + shell_quote(script) + " " + shell_quote(query) +
                      " " + std::to_string(count);
    ProcResult r = run_capture(cmd);
    return parse_output(r.out, count);
}

std::vector<AudiusTrack> AudiusSource::trending(int count) {
    std::string script = (scripts_dir_ / "audius_search.py").string();
    std::string cmd = "python3 " + shell_quote(script) + " trending " + std::to_string(count);
    ProcResult r = run_capture(cmd);
    return parse_output(r.out, count);
}

std::string AudiusSource::stream_url(const std::string& track_id) {
    std::string script = (scripts_dir_ / "audius_search.py").string();
    std::string cmd = "python3 " + shell_quote(script) + " stream " + shell_quote(track_id);
    ProcResult r = run_capture(cmd);
    // Trim whitespace/newlines
    std::string url = r.out;
    while (!url.empty() && (url.back() == '\n' || url.back() == '\r' || url.back() == ' '))
        url.pop_back();
    return url;
}

} // namespace muisc

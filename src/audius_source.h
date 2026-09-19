#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace muisc {

namespace fs = std::filesystem;

struct AudiusTrack {
    std::string id;
    std::string title;
    std::string uploader;
    double duration_sec = 0.0;
};

class AudiusSource {
public:
    explicit AudiusSource(fs::path scripts_dir) : scripts_dir_(std::move(scripts_dir)) {}

    // Search Audius for tracks matching query
    std::vector<AudiusTrack> search(const std::string& query, int count = 10);

    // Fetch trending tracks from Audius
    std::vector<AudiusTrack> trending(int count = 10);

    // Get direct CDN stream URL for a track id (follows 302 redirect)
    // Returns empty string on failure.
    std::string stream_url(const std::string& track_id);

private:
    fs::path scripts_dir_;

    std::vector<AudiusTrack> parse_output(const std::string& raw, int limit);
};

} // namespace muisc

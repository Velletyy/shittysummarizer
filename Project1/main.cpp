#include <iostream>
#include <fstream>
#include <filesystem>
#include <windows.h> 
#include "json.hpp"
#include <string>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::json;

const std::string ITEMID_JADEFOREST         = "44490_0";
const std::string ITEMID_DARKSEEKERS        = "65330_0";
const std::string ITEMID_TUNGRAD            = "65328_0";
const std::string ITEMID_CRYPT              = "44450_0";
const std::string ITEMID_THORNWOOD          = "44451_0";
const std::string ITEMID_ASH_FOREST         = "44411_0";
const std::string ITEMID_TROLLS             = "59798_0";
const std::string ITEMID_CYCLOPS_DEHKIA     = "44522_0";
const std::string ITEMID_THORNWOOD_DEHKIA   = "44520_0";
const std::string ITEMID_TUNKUTA_DEHKIA     = "44521_0";
const std::string ITEMID_ASH_FOREST_DEHKIA  = "44518_0";
const std::string ITEMID_OLUNS_DEHKIA       = "44519_0";
const std::string ITEMID_AAKMAN_DEHKIA      = "65400_0";
const std::string ITEMID_GYFIN_UG           = "44516_0";
const std::string ITEMID_GIANTS             = "59799_0";
const std::string ITEMID_ORCS               = "44482_0";
const std::string ITEMID_COTD               = "65329_0";
const std::string ITEMID_STARS_END          = "44400_0";
const std::string ITEMID_SYCRAIA_LOWER      = "44380_0";
const std::string ITEMID_KRATUGA            = "44423_0";
const std::string ITEMID_TUNKUTA            = "44454_0";
const std::string ITEMID_WARAGON            = "721048_0";

// To encapsulate data
class GrindSpot {
public:
    int id;
    std::string name;
    std::string key;

    GrindSpot(int id, const std::string& name, const std::string& key) : id(id), name(name), key(key) {}
};

// Enum for session status
enum class SessionStatus {
    VALID,
    INVALID_FILE,
    JSON_PARSE_ERROR,
    MISSING_KEY,
    INVALID_GRINDSPOT,
};

// Session structure
struct Session {
    GrindSpot grindSpot;
    int value; // <-- what we want to extract, aka trash loot
    time_t creationTime;
    SessionStatus status = SessionStatus::VALID; // default to valid
    std::string filePath;

    Session(const GrindSpot& grindSpot, int value, const std::string& filePath) :
        grindSpot(grindSpot), value(value), creationTime(time(nullptr)), filePath(filePath) {}
};

// To set the console output color
void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

// To sort sessions based on grindspot
bool compareSessions(const Session& session1, const Session& session2) {
    return session1.grindSpot.id < session2.grindSpot.id;
}

// To get the grindspot name
std::string getGrindspotName(const GrindSpot& grindSpot) {
    return grindSpot.name;
}

// To load the data we need
std::vector<GrindSpot> loadGrindSpots() {
    return {
        {110,   "Starlight Jade Forest",            ITEMID_JADEFOREST},
        {153,   "Darkseeker's Retreat",             ITEMID_DARKSEEKERS},
        {148,   "Tungrad Ruins",                    ITEMID_TUNGRAD},
        {8,     "Crypt of Resting Thoughts",        ITEMID_CRYPT},
        {7,     "Thornwood Forest",                 ITEMID_THORNWOOD},
        {27,    "Ash Forest",                       ITEMID_ASH_FOREST},
        {121,   "Troll Habitat",                    ITEMID_TROLLS},
        {151,   "[Dehkia] Cyclops Land",            ITEMID_CYCLOPS_DEHKIA},
        {146,   "[Dehkia] Thornwood Forest",        ITEMID_THORNWOOD_DEHKIA},
        {145,   "[Dehkia] Tunkuta",                 ITEMID_TUNKUTA_DEHKIA},
        {143,   "[Dehkia] Ash Forest",              ITEMID_ASH_FOREST_DEHKIA},
        {144,   "[Dehkia] Olun's Valley",           ITEMID_OLUNS_DEHKIA},
        {155,   "[Dehkia] Aakman Temple",           ITEMID_AAKMAN_DEHKIA},
        {97,    "Gyfin Rhasia Underground",         ITEMID_GYFIN_UG},
        {120,   "Primal Giant Post",                ITEMID_GIANTS},
        {17,    "Orc Camp",                         ITEMID_ORCS},
        {147,   "City of the Dead",                 ITEMID_COTD},
        {2,     "Star's End",                       ITEMID_STARS_END},
        {1,     "Sycraia Abyssal Ruins (Lower)",    ITEMID_SYCRAIA_LOWER},
        {9,     "Kratuga Ancient Ruins",            ITEMID_KRATUGA},
        {4,     "Tunkuta",                          ITEMID_TUNKUTA},
        {88,    "Waragon Nest",                     ITEMID_WARAGON}
    };
}

// To extract the trash loot from the grind reports
Session extractAndValidateSession(const std::string& filePath, const std::vector<GrindSpot>& grindSpots) {
    std::ifstream file(filePath);
    Session session = Session(GrindSpot(-1, "Unknown", ""), -1, filePath);

    if (file.is_open()) {
        try {
            json jsonData;
            file >> jsonData;

            int grindspot_id = jsonData["grindspot_id"];
            auto grindSpot = std::find_if(grindSpots.begin(), grindSpots.end(),
                [&](const GrindSpot& spot) { return spot.id == grindspot_id; });

            if (grindSpot != grindSpots.end() && jsonData.contains("newSession") && jsonData["newSession"].contains("drops")) {
                int value = jsonData["newSession"]["drops"][grindSpot->key];
                return Session(*grindSpot, value, filePath);
            }
            else if (grindSpot == grindSpots.end()) {
                session.status = SessionStatus::INVALID_GRINDSPOT;
                return session;
            }
            else {
                session.status = SessionStatus::MISSING_KEY;
                return session;
            }
        }
        catch (const json::exception& e) {
            session.status = SessionStatus::JSON_PARSE_ERROR;
            return session;
        }
    }
    else {
        session.status = SessionStatus::INVALID_FILE;
        return session;
    }
}

// the main thing, the output..
void displaySummary(const std::vector<Session>& sessions, int sessionCounter) {
    std::unordered_map<int, int> bestSessionIndexMap;
    std::unordered_map<int, int> highestCountMap;

    std::vector<std::pair<std::string, SessionStatus>> errorSessions;

    std::string currentGrindspot = "";
    int currentGrindspotIndex = 0;

    for (size_t i = 0; i < sessions.size(); ++i) {
        const auto& session = sessions[i];

        if (session.status != SessionStatus::VALID) {
            errorSessions.push_back({ session.filePath, session.status });
            continue;
        }

        if (currentGrindspot != getGrindspotName(session.grindSpot)) {
            setColor(14); // Yellow
            if (currentGrindspotIndex > 0) {
                std::cout << '\n';
            }
            std::cout << ">>> " << getGrindspotName(session.grindSpot) << " <<<\n";
            setColor(15); // White
            currentGrindspot = getGrindspotName(session.grindSpot);
            currentGrindspotIndex = 0;
        }

        setColor(11); // Cyan
        std::cout << "#" << ++currentGrindspotIndex << " ";
        setColor(10); // Green
        std::cout << session.value << " [";
        std::cout << session.value / 60 << "/min]";
        setColor(11); // Cyan
        std::cout << "\n";

        // Track the best session/hour for each location
        if (session.value > highestCountMap[session.grindSpot.id]) {
            highestCountMap[session.grindSpot.id] = session.value;
            bestSessionIndexMap[session.grindSpot.id] = i;
        }
    }

    // Output any culprits
    if (!errorSessions.empty()) {
        setColor(12); // Red
        std::cout << "\nERRORS:\n";
        for (const auto& [filename, errorStatus] : errorSessions) {
            std::cout << filename << ": [";

            switch (errorStatus) {
            case SessionStatus::MISSING_KEY:
                std::cout << "Missing Key";
                break;
            case SessionStatus::INVALID_GRINDSPOT:
                std::cout << "Unsupported Grind Spot";
                break;
            default:
                std::cout << "Unknown, missing drop id?";
            }
            std::cout << "]\n";
        }
        setColor(11); // Cyan
    }

    std::cout << "\nTotal sessions: #" << sessionCounter << "\n";

    // Output the best session/hour for each location
    for (const auto& entry : bestSessionIndexMap) {
        const auto& session = sessions[entry.second];
        int highestCount = highestCountMap[session.grindSpot.id];

        setColor(11); // Cyan
        std::cout << "Best hour for ";
        setColor(14); // Yellow
        std::cout << getGrindspotName(session.grindSpot);
        setColor(11); // Cyan
        std::cout << ": ";
        setColor(10); // Green
        std::cout << highestCount;
        setColor(11); // Cyan
        std::cout << " [";
        setColor(10); // Green
        std::cout << highestCount / 60;
        setColor(11); // Cyan
        std::cout << "/min]\n";
    }

    setColor(15); // White
}

void processAllJSONFiles(std::vector<Session>& sessions, int& sessionCounter, const std::vector<GrindSpot>& grindSpots) {
    for (const auto& entry : fs::directory_iterator(".")) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            Session session = extractAndValidateSession(entry.path().string(), grindSpots);
            sessions.push_back(session);

            // Increment the counter if it's a valid session
            if (session.status == SessionStatus::VALID) {
                ++sessionCounter;
            }
        }
    }

    std::sort(sessions.begin(), sessions.end(), [](const Session& s1, const Session& s2) {
        if (s1.grindSpot.id == s2.grindSpot.id) {
            return s1.creationTime < s2.creationTime;
        }
        return s1.grindSpot.id < s2.grindSpot.id;
        });
}

int main() {
    std::vector<Session> sessions;
    int sessionCounter = 0;
    auto grindSpots = loadGrindSpots();

    processAllJSONFiles(sessions, sessionCounter, grindSpots);

    if (sessions.empty()) {
        setColor(12); // Red
        std::cerr << "No valid data found. Ensure .json files exist with the correct structure.\n";
        setColor(15); // White
    }
    else {
        displaySummary(sessions, sessionCounter);
    }

    setColor(15); // White
    std::cout << "\nPress 'ENTER' to exit..";
    std::cin.ignore();

    return 0;
}
// JUIT Club Hub backend.
// Plain C++11 with plain sockets and plain classes.
// Every club, event, stall, sponsor and post below is sample data.

#include <iostream>
#include <vector>
#include <queue>
#include <deque>
#include <map>
#include <algorithm>
#include <string>
#include <sstream>
#include <fstream>
#include <ctime>

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET SocketType;
typedef int SockLen;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int SocketType;
typedef socklen_t SockLen;
#endif

using namespace std;

// Removes spaces from both ends of a text.
string trim(const string& s) {
  size_t start = 0;
  size_t end = s.size();
  while (start < end && (s[start] == ' ' || s[start] == '\t')) start = start + 1;
  while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) end = end - 1;
  return s.substr(start, end - start);
}

// Escapes quotes and backslashes so text fits inside a JSON string.
string esc(const string& s) {
  string out = "";
  for (size_t i = 0; i < s.size(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') {
      out += '\\';
      out += c;
    } else if (c == '\n') {
      out += "\\n";
    } else if (c == '\r') {
      // skip carriage returns
    } else {
      out += c;
    }
  }
  return out;
}

// Turns one number into text.
string numText(int n) {
  stringstream ss;
  ss << n;
  return ss.str();
}

// Returns a date string like 2026-03-14 for today plus some days.
string dateAfter(int days) {
  time_t now = time(0) + (time_t)days * 86400;
  tm* info = localtime(&now);
  stringstream ss;
  int year = info->tm_year + 1900;
  int month = info->tm_mon + 1;
  int day = info->tm_mday;
  ss << year << "-";
  if (month < 10) ss << "0";
  ss << month << "-";
  if (day < 10) ss << "0";
  ss << day;
  return ss.str();
}

// Turns text into lowercase.
string lower(string s) {
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] >= 'A' && s[i] <= 'Z') s[i] = (char)(s[i] - 'A' + 'a');
  }
  return s;
}

// Value of one hex character.
int hexValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return 0;
}

// Turns %20 and plus signs back into normal characters.
string urlDecode(const string& s) {
  string out = "";
  for (size_t i = 0; i < s.size(); i++) {
    char c = s[i];
    if (c == '+') {
      out += ' ';
    } else if (c == '%' && i + 2 < s.size()) {
      int value = hexValue(s[i + 1]) * 16 + hexValue(s[i + 2]);
      out += (char)value;
      i = i + 2;
    } else {
      out += c;
    }
  }
  return out;
}

// Reads one field from text like club=a&student=b.
string formValue(const string& text, const string& key) {
  stringstream ss(text);
  string pair;
  while (getline(ss, pair, '&')) {
    size_t eq = pair.find('=');
    if (eq == string::npos) continue;
    string name = urlDecode(pair.substr(0, eq));
    if (name == key) return urlDecode(pair.substr(eq + 1));
  }
  return "";
}

// Checks one text against a fixed list.
bool inList(const vector<string>& list, const string& value) {
  for (size_t i = 0; i < list.size(); i++) {
    if (list[i] == value) return true;
  }
  return false;
}

// The fixed list of audition venues from the college.
vector<string> auditionVenues() {
  vector<string> list;
  for (int i = 1; i <= 12; i++) {
    stringstream s;
    s << "CR" << i;
    list.push_back(s.str());
  }
  for (int i = 1; i <= 9; i++) {
    stringstream s;
    s << "TR" << i;
    list.push_back(s.str());
  }
  list.push_back("Auditorium");
  list.push_back("LT2");
  list.push_back("LT3");
  list.push_back("DLC");
  return list;
}

// Splits a text like singing|dancing into a list.
vector<string> splitSkills(const string& text) {
  vector<string> list;
  stringstream ss(text);
  string part;
  while (getline(ss, part, '|')) {
    list.push_back(trim(part));
  }
  return list;
}

// Reads a whole file into one string.
string readFile(const string& name) {
  ifstream file(name.c_str(), ios::in | ios::binary);
  if (!file) return "";
  stringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

// Tells if a socket value is bad.
bool socketBad(SocketType s) {
#ifdef _WIN32
  return s == INVALID_SOCKET;
#else
  return s < 0;
#endif
}

// Closes one socket.
void closeSocket(SocketType s) {
#ifdef _WIN32
  closesocket(s);
#else
  close(s);
#endif
}

// One item on a stall menu.
class MenuItem {
public:
  string name;
  int price;
  MenuItem() { price = 0; }
  MenuItem(string n, int p) {
    name = n;
    price = p;
  }
};

// One club with its sample details.
class Club {
public:
  string name;
  string group;
  string description;
  string handle;
  vector<string> skills;
  bool auditionOpen;
  string venue;
  string date;
  string time;
  string contactName;
  string phone;
  string email;
  Club() { auditionOpen = false; }
};

// One event. startDay is days from today and length is days.
class Event {
public:
  int id;
  string name;
  vector<string> hosts;
  string venue;
  int startDay;
  int length;
  string description;
  string skill;
  string contactName;
  string phone;
  string email;
  vector<int> sponsorIndexes;
  Event() {
    id = 0;
    startDay = 0;
    length = 1;
  }
};

// One food stall with its menu.
class Stall {
public:
  string name;
  string location;
  string timing;
  int eventId;
  vector<MenuItem> menu;
  Stall() { eventId = 0; }
};

// One sponsor.
class Sponsor {
public:
  string name;
  string info;
  string website;
  string events;
};

// One instagram style post.
class Post {
public:
  string handle;
  string caption;
  string date;
};

// One audition match for the priority queue.
class ClubMatch {
public:
  int clubIndex;
  int matchCount;
  string clubName;
  ClubMatch() {
    clubIndex = 0;
    matchCount = 0;
  }
  // More matching skills means higher priority, ties go by club name.
  bool operator<(const ClubMatch& other) const {
    if (matchCount != other.matchCount) return matchCount < other.matchCount;
    return clubName > other.clubName;
  }
};

// One HTTP response: code, content type and body.
class Response {
public:
  int code;
  string type;
  string body;
  Response() {
    code = 200;
    type = "application/json";
  }
};

// Returns upcoming, ongoing or completed for one event today.
string eventStatus(const Event& e) {
  if (e.startDay > 0) return "upcoming";
  if (e.startDay + e.length > 0) return "ongoing";
  return "completed";
}

// Used by sort() to put the nearest start date first.
bool earlierEvent(const Event& a, const Event& b) {
  return a.startDay < b.startDay;
}

// A list with one sponsor index.
vector<int> oneIndex(int a) {
  vector<int> list;
  list.push_back(a);
  return list;
}

// A list with two sponsor indexes.
vector<int> twoIndexes(int a, int b) {
  vector<int> list;
  list.push_back(a);
  list.push_back(b);
  return list;
}

// The server keeps all data and answers one request at a time.
class Server {
public:
  vector<Club> clubs;
  vector<Event> events;
  vector<Stall> stalls;
  vector<Sponsor> sponsors;
  vector<Post> posts;
  map<string, vector<int> > skillIndex;
  map<int, vector<int> > stallsByEvent;
  queue<string> interestQueue;
  deque<string> recentSearches;

  // Adds one club to the club list.
  void addClub(string name, string group, string description, string handle,
               vector<string> skills, bool auditionOpen, string venue,
               string date, string time, string contactName, string phone,
               string email) {
    Club c;
    c.name = name;
    c.group = group;
    c.description = description;
    c.handle = handle;
    c.skills = skills;
    c.auditionOpen = auditionOpen;
    c.venue = venue;
    c.date = date;
    c.time = time;
    c.contactName = contactName;
    c.phone = phone;
    c.email = email;
    clubs.push_back(c);
  }

  // Adds one event to the event list.
  void addEvent(int id, string name, vector<string> hosts, string venue,
                int startDay, int length, string description, string skill,
                string contactName, string phone, string email,
                vector<int> sponsorIndexes) {
    Event e;
    e.id = id;
    e.name = name;
    e.hosts = hosts;
    e.venue = venue;
    e.startDay = startDay;
    e.length = length;
    e.description = description;
    e.skill = skill;
    e.contactName = contactName;
    e.phone = phone;
    e.email = email;
    e.sponsorIndexes = sponsorIndexes;
    events.push_back(e);
  }

  // Adds one stall to the stall list.
  void addStall(string name, string location, string timing, int eventId,
                vector<MenuItem> menu) {
    Stall s;
    s.name = name;
    s.location = location;
    s.timing = timing;
    s.eventId = eventId;
    s.menu = menu;
    stalls.push_back(s);
  }

  // Adds one sponsor to the sponsor list.
  void addSponsor(string name, string info, string website, string eventList) {
    Sponsor s;
    s.name = name;
    s.info = info;
    s.website = website;
    s.events = eventList;
    sponsors.push_back(s);
  }

  // Adds one instagram style post.
  void addPost(string handle, string caption, string date) {
    Post p;
    p.handle = handle;
    p.caption = caption;
    p.date = date;
    posts.push_back(p);
  }

  // Fills all sample lists. Change the sample details here.
  void addSampleData() {
    // Sponsors come first because events point to them by index.
    addSponsor("Waknaghat Motors", "Local bike and car service partner near the campus gate.", "https://example.com/waknaghat-motors", "Alfaz, 3v3 Basketball Meet");
    addSponsor("Cafe Terra", "Small cafe on the Waknaghat road with study tables.", "https://example.com/cafe-terra", "Alfaz, Open Mic Evening");
    addSponsor("Himalaya Prints", "Printing, banners and club merchandise in Solan.", "https://example.com/himalaya-prints", "HackJUIT Hackathon, Debate Night");
    addSponsor("Green Trail Adventures", "Trekking and outdoor gear rental.", "https://example.com/green-trail", "3v3 Basketball Meet, Photography Walk");
    addSponsor("Solan Tech Store", "Laptops, spare parts and repairs.", "https://example.com/solan-tech", "HackJUIT Hackathon");

    // JYC clubs.
    addClub("Cultural and Music Club", "JYC", "Music, singing and stage events for the whole campus.", "culturalandmusic.juit", splitSkills("singing|instrumental music|event management"), true, "Auditorium", dateAfter(3), "4:00 PM", "Ananya Sharma", "98000 00001", "cultural.music@example.com");
    addClub("Theatre Club", "JYC", "Street plays, stage plays and open mic evenings.", "theatre.juit", splitSkills("acting|public speaking|writing"), true, "LT2", dateAfter(4), "3:30 PM", "Rohan Verma", "98000 00002", "theatre@example.com");
    addClub("Literary and Debating Club", "JYC", "Debates, quizzes and writing circles.", "litdebate.juit", splitSkills("debating|public speaking|writing"), true, "CR5", dateAfter(5), "2:00 PM", "Meera Nair", "98000 00003", "litdebate@example.com");
    addClub("Dance Club", "JYC", "Group and solo dance practice, plus campus performances.", "dance.juit", splitSkills("dancing|fitness"), true, "Auditorium", dateAfter(6), "5:00 PM", "Kabir Singh", "98000 00004", "dance@example.com");
    addClub("Arts Club", "JYC", "Painting, sketching and campus art shows.", "arts.juit", splitSkills("painting|photography"), true, "CR7", dateAfter(7), "11:00 AM", "Ishita Rao", "98000 00005", "arts@example.com");
    addClub("Health Club", "JYC", "Health camps, first aid and wellness talks.", "health.juit", splitSkills("fitness|event management"), false, "CR3", dateAfter(8), "10:00 AM", "Nitin Bose", "98000 00006", "health@example.com");
    addClub("Sports Club", "JYC", "Campus leagues for basketball, football and athletics.", "sports.juit", splitSkills("sports|fitness|event management"), true, "DLC", dateAfter(9), "4:00 PM", "Arjun Thakur", "98000 00007", "sports@example.com");
    addClub("Koshish Club", "JYC", "Teaching and support work for underprivileged children.", "koshish.juit", splitSkills("social service|public speaking|event management"), true, "CR9", dateAfter(10), "1:00 PM", "Priya Chauhan", "98000 00008", "koshish@example.com");
    addClub("Fit India Club", "JYC", "Morning runs, fitness challenges and yoga sessions.", "fitindia.juit", splitSkills("fitness|sports"), false, "TR4", dateAfter(11), "7:00 AM", "Vikram Rana", "98000 00009", "fitindia@example.com");

    // Technical clubs and student branches.
    addClub("IEEE JUIT Student Branch", "Technical", "Student branch for electronics and computing talks and papers.", "ieee.juit", splitSkills("coding|electronics|web development"), true, "LT3", dateAfter(4), "3:00 PM", "Aditya Kulkarni", "98000 00010", "ieee@example.com");
    addClub("ACM JUIT Student Chapter", "Technical", "Computing chapter with contests, workshops and study groups.", "acm.juit", splitSkills("coding|web development|event management"), true, "CR11", dateAfter(5), "4:30 PM", "Sneha Iyer", "98000 00011", "acm@example.com");
    addClub("SIAM JUIT", "Technical", "Applied maths and modelling group with reading sessions.", "siam.juit", splitSkills("coding|writing"), true, "TR6", dateAfter(6), "12:00 PM", "Rahul Menon", "98000 00012", "siam@example.com");
    addClub("Createch", "Technical", "Makers club for hardware, robots and editing work.", "createch.juit", splitSkills("electronics|coding|video editing"), true, "DLC", dateAfter(7), "2:30 PM", "Tara Kapoor", "98000 00013", "createch@example.com");
    addClub("Technical and Sciences Club", "Technical", "Science demos, talks and lab based projects.", "techsci.juit", splitSkills("coding|electronics|writing"), false, "CR2", dateAfter(8), "11:30 AM", "Mohit Salvi", "98000 00014", "techsci@example.com");
    addClub("Rotaract Club of Waknaghat", "Technical", "Service projects and community drives around Waknaghat.", "rotaract.waknaghat", splitSkills("social service|event management|public speaking"), true, "TR8", dateAfter(9), "3:00 PM", "Nidhi Bhardwaj", "98000 00015", "rotaract@example.com");
    addClub("Omega Leo Club", "Technical", "Youth service club with clean up and donation drives.", "omegaleo.waknaghat", splitSkills("social service|event management"), true, "CR6", dateAfter(10), "10:30 AM", "Sahil Dhiman", "98000 00016", "omegaleo@example.com");
    addClub("Gender Champions Club", "Technical", "Awareness sessions and campus safety projects.", "genderchampions.juit", splitSkills("public speaking|writing|social service"), true, "CR8", dateAfter(11), "1:30 PM", "Ritika Sen", "98000 00017", "genderchampions@example.com");

    // Events. One event is running today, the rest start in the next days.
    // Event name is sample, replace with the real name.
    addEvent(1, "Alfaz", splitSkills("Cultural and Music Club|Theatre Club"), "Auditorium", -1, 3,
             "Three evenings of music and spoken word with student bands and solo sets.",
             "No skill needed (singing or instrumental music for performers)",
             "Ananya Sharma", "98000 00001", "cultural.music@example.com", twoIndexes(0, 1));
    addEvent(2, "HackJUIT Hackathon", splitSkills("IEEE JUIT Student Branch|ACM JUIT Student Chapter"), "DLC", 5, 2,
             "A long build sprint on campus tools and open data, teams of four.",
             "coding", "Aditya Kulkarni", "98000 00010", "ieee@example.com", twoIndexes(2, 4));
    addEvent(3, "Debate Night", splitSkills("Literary and Debating Club"), "LT2", 3, 1,
             "Knockout debates on campus and national motions.",
             "public speaking", "Meera Nair", "98000 00003", "litdebate@example.com", oneIndex(2));
    addEvent(4, "3v3 Basketball Meet", splitSkills("Sports Club"), "Basketball Court", 8, 2,
             "Half court three on three league with evening finals.",
             "sports", "Arjun Thakur", "98000 00007", "sports@example.com", twoIndexes(0, 3));
    addEvent(5, "Open Mic Evening", splitSkills("Theatre Club|Cultural and Music Club"), "Auditorium", 12, 1,
             "Five minute slots for songs, poems and short acts.",
             "singing", "Rohan Verma", "98000 00002", "theatre@example.com", oneIndex(1));
    addEvent(6, "Photography Walk", splitSkills("Arts Club"), "TR3", 15, 1,
             "A slow walk around campus with photo prompts and a print review.",
             "photography", "Ishita Rao", "98000 00005", "arts@example.com", oneIndex(3));

    // Stalls with menus in rupees.
    vector<MenuItem> menu1;
    menu1.push_back(MenuItem("Kulhad chai", 30));
    menu1.push_back(MenuItem("Adrak chai", 25));
    menu1.push_back(MenuItem("Bun maska", 40));
    menu1.push_back(MenuItem("Samosa", 25));
    menu1.push_back(MenuItem("Maggi", 50));
    addStall("Chai Point", "Mughal Garden", "11:00 AM to 8:00 PM", 1, menu1);

    vector<MenuItem> menu2;
    menu2.push_back(MenuItem("Veg momo", 60));
    menu2.push_back(MenuItem("Chicken momo", 80));
    menu2.push_back(MenuItem("Fried momo", 70));
    menu2.push_back(MenuItem("Mayo dip", 20));
    menu2.push_back(MenuItem("Cold drink", 40));
    addStall("Momo Junction", "OC", "12:00 PM to 9:00 PM", 1, menu2);

    vector<MenuItem> menu3;
    menu3.push_back(MenuItem("Paneer wrap", 90));
    menu3.push_back(MenuItem("Chicken wrap", 110));
    menu3.push_back(MenuItem("Veg roll", 70));
    menu3.push_back(MenuItem("Lemonade", 40));
    menu3.push_back(MenuItem("Brownie", 60));
    addStall("Wrap and Roll", "OC", "1:00 PM to 8:00 PM", 2, menu3);

    vector<MenuItem> menu4;
    menu4.push_back(MenuItem("Cold coffee", 70));
    menu4.push_back(MenuItem("Veg sandwich", 80));
    menu4.push_back(MenuItem("Fruit bowl", 60));
    menu4.push_back(MenuItem("Energy drink", 90));
    menu4.push_back(MenuItem("Water bottle", 20));
    addStall("Courtside Coolers", "OC", "8:00 AM to 6:00 PM", 4, menu4);

    vector<MenuItem> menu5;
    menu5.push_back(MenuItem("Golgappa", 40));
    menu5.push_back(MenuItem("Aloo tikki", 50));
    menu5.push_back(MenuItem("Bhel puri", 40));
    menu5.push_back(MenuItem("Dahi bhalla", 60));
    menu5.push_back(MenuItem("Lassi", 50));
    addStall("Dilli Chaat", "OC", "4:00 PM to 10:00 PM", 3, menu5);

    vector<MenuItem> menu6;
    menu6.push_back(MenuItem("Cupcake", 50));
    menu6.push_back(MenuItem("Cookie", 30));
    menu6.push_back(MenuItem("Brownie", 60));
    menu6.push_back(MenuItem("Iced tea", 40));
    menu6.push_back(MenuItem("Cheese toast", 70));
    addStall("Book Nook Bakes", "Mughal Garden", "3:00 PM to 9:00 PM", 5, menu6);

    // Instagram style posts. There is no live Instagram connection.
    addPost("culturalandmusic.juit", "Alfaz is on at the Auditorium. Stalls at Mughal Garden and OC are open till Sunday night.", dateAfter(-1));
    addPost("ieee.juit", "Hackathon registrations are open. Teams of four, bring your own laptops.", dateAfter(0));
    addPost("litdebate.juit", "Debate Night motions go up on Friday. Practice rounds at LT2 every evening.", dateAfter(1));
    addPost("arts.juit", "The photography walk list is out. New members welcome.", dateAfter(2));
    addPost("sports.juit", "3v3 basketball court bookings start tomorrow, sign up at OC.", dateAfter(2));
    addPost("koshish.juit", "Weekend teaching slots are open. Two hours a week is enough to help.", dateAfter(3));
    addPost("rotaract.waknaghat", "Clean up drive near the main gate this Sunday morning.", dateAfter(4));
  }

  // Builds the skill map and the event to stall map once at startup.
  void buildIndexes() {
    skillIndex.clear();
    for (size_t i = 0; i < clubs.size(); i++) {
      for (size_t k = 0; k < clubs[i].skills.size(); k++) {
        skillIndex[clubs[i].skills[k]].push_back((int)i);
      }
    }
    stallsByEvent.clear();
    for (size_t i = 0; i < stalls.size(); i++) {
      stallsByEvent[stalls[i].eventId].push_back((int)i);
    }
  }

  // Checks every venue against the fixed lists and warns if one is wrong.
  void checkVenues() {
    vector<string> aud = auditionVenues();
    vector<string> stallPlaces;
    stallPlaces.push_back("Mughal Garden");
    stallPlaces.push_back("OC");
    vector<string> eventPlaces = aud;
    eventPlaces.push_back("Basketball Court");
    for (size_t i = 0; i < clubs.size(); i++) {
      if (!inList(aud, clubs[i].venue)) {
        cout << "Venue warning: club " << clubs[i].name << " uses " << clubs[i].venue << endl;
      }
    }
    for (size_t i = 0; i < stalls.size(); i++) {
      if (!inList(stallPlaces, stalls[i].location)) {
        cout << "Venue warning: stall " << stalls[i].name << " uses " << stalls[i].location << endl;
      }
    }
    for (size_t i = 0; i < events.size(); i++) {
      if (!inList(eventPlaces, events[i].venue)) {
        cout << "Venue warning: event " << events[i].name << " uses " << events[i].venue << endl;
      }
    }
  }

  // JSON for one sponsor.
  string sponsorJson(int index) {
    Sponsor& s = sponsors[index];
    stringstream j;
    j << "{";
    j << "\"name\":\"" << esc(s.name) << "\",";
    j << "\"info\":\"" << esc(s.info) << "\",";
    j << "\"website\":\"" << esc(s.website) << "\",";
    j << "\"events\":\"" << esc(s.events) << "\"";
    j << "}";
    return j.str();
  }

  // JSON for one event, with its stalls and sponsors.
  string eventJson(const Event& e) {
    stringstream j;
    j << "{";
    j << "\"id\":" << e.id << ",";
    j << "\"name\":\"" << esc(e.name) << "\",";
    j << "\"status\":\"" << eventStatus(e) << "\",";
    j << "\"hosts\":[";
    for (size_t i = 0; i < e.hosts.size(); i++) {
      if (i > 0) j << ",";
      j << "\"" << esc(e.hosts[i]) << "\"";
    }
    j << "],";
    j << "\"venue\":\"" << esc(e.venue) << "\",";
    j << "\"startDate\":\"" << dateAfter(e.startDay) << "\",";
    j << "\"endDate\":\"" << dateAfter(e.startDay + e.length - 1) << "\",";
    j << "\"description\":\"" << esc(e.description) << "\",";
    j << "\"skill\":\"" << esc(e.skill) << "\",";
    j << "\"contact\":{\"name\":\"" << esc(e.contactName) << "\",";
    j << "\"phone\":\"" << esc(e.phone) << "\",";
    j << "\"email\":\"" << esc(e.email) << "\"},";
    j << "\"stalls\":[";
    vector<int> stallList = stallsByEvent[e.id];
    for (size_t s = 0; s < stallList.size(); s++) {
      Stall& st = stalls[stallList[s]];
      if (s > 0) j << ",";
      j << "{";
      j << "\"name\":\"" << esc(st.name) << "\",";
      j << "\"location\":\"" << esc(st.location) << "\",";
      j << "\"timing\":\"" << esc(st.timing) << "\",";
      j << "\"menu\":[";
      for (size_t m = 0; m < st.menu.size(); m++) {
        if (m > 0) j << ",";
        j << "{\"item\":\"" << esc(st.menu[m].name) << "\",";
        j << "\"price\":" << st.menu[m].price << "}";
      }
      j << "]}";
    }
    j << "],";
    j << "\"sponsors\":[";
    for (size_t k = 0; k < e.sponsorIndexes.size(); k++) {
      if (k > 0) j << ",";
      j << sponsorJson(e.sponsorIndexes[k]);
    }
    j << "]}";
    return j.str();
  }

  // JSON for one ranked audition club.
  string auditionClubJson(const ClubMatch& m) {
    Club& c = clubs[m.clubIndex];
    stringstream j;
    j << "{";
    j << "\"name\":\"" << esc(c.name) << "\",";
    j << "\"group\":\"" << esc(c.group) << "\",";
    j << "\"matchCount\":" << m.matchCount << ",";
    j << "\"skills\":[";
    for (size_t i = 0; i < c.skills.size(); i++) {
      if (i > 0) j << ",";
      j << "\"" << esc(c.skills[i]) << "\"";
    }
    j << "],";
    j << "\"venue\":\"" << esc(c.venue) << "\",";
    j << "\"date\":\"" << esc(c.date) << "\",";
    j << "\"time\":\"" << esc(c.time) << "\",";
    j << "\"contactName\":\"" << esc(c.contactName) << "\",";
    j << "\"phone\":\"" << esc(c.phone) << "\",";
    j << "\"email\":\"" << esc(c.email) << "\"";
    j << "}";
    return j.str();
  }

  // Home page data: running events plus the next three upcoming events.
  string homeJson() {
    vector<Event> live;
    vector<Event> next;
    for (size_t i = 0; i < events.size(); i++) {
      string st = eventStatus(events[i]);
      if (st == "ongoing") live.push_back(events[i]);
      else if (st == "upcoming") next.push_back(events[i]);
    }
    sort(next.begin(), next.end(), earlierEvent);
    if (next.size() > 3) next.resize(3);
    string out = "";
    for (size_t i = 0; i < live.size(); i++) {
      if (out != "") out += ",";
      out += eventJson(live[i]);
    }
    for (size_t i = 0; i < next.size(); i++) {
      if (out != "") out += ",";
      out += eventJson(next[i]);
    }
    return "{\"events\":[" + out + "]}";
  }

  // Event list for one status, sorted by the start date.
  string eventsJson(const string& status) {
    vector<Event> list;
    for (size_t i = 0; i < events.size(); i++) {
      if (eventStatus(events[i]) == status) list.push_back(events[i]);
    }
    sort(list.begin(), list.end(), earlierEvent);
    string out = "";
    for (size_t i = 0; i < list.size(); i++) {
      if (out != "") out += ",";
      out += eventJson(list[i]);
    }
    return "{\"events\":[" + out + "]}";
  }

  // All clubs with their contact details.
  string clubsJson() {
    string out = "";
    for (size_t i = 0; i < clubs.size(); i++) {
      Club& c = clubs[i];
      if (i > 0) out += ",";
      stringstream j;
      j << "{";
      j << "\"name\":\"" << esc(c.name) << "\",";
      j << "\"group\":\"" << esc(c.group) << "\",";
      j << "\"description\":\"" << esc(c.description) << "\",";
      j << "\"handle\":\"" << esc(c.handle) << "\",";
      j << "\"skills\":[";
      for (size_t s = 0; s < c.skills.size(); s++) {
        if (s > 0) j << ",";
        j << "\"" << esc(c.skills[s]) << "\"";
      }
      j << "],";
      j << "\"auditionOpen\":" << (c.auditionOpen ? "true" : "false") << ",";
      j << "\"venue\":\"" << esc(c.venue) << "\",";
      j << "\"date\":\"" << esc(c.date) << "\",";
      j << "\"time\":\"" << esc(c.time) << "\",";
      j << "\"contactName\":\"" << esc(c.contactName) << "\",";
      j << "\"phone\":\"" << esc(c.phone) << "\",";
      j << "\"email\":\"" << esc(c.email) << "\"";
      j << "}";
      out += j.str();
    }
    return "{\"clubs\":[" + out + "]}";
  }

  // Ranks open audition clubs for the chosen skills using the priority queue.
  string auditionsJson(const string& skillsParam) {
    vector<string> wanted;
    stringstream ss(skillsParam);
    string part;
    while (getline(ss, part, ',')) {
      string clean = trim(part);
      if (clean != "") wanted.push_back(clean);
    }

    map<int, int> counts;
    for (size_t i = 0; i < wanted.size(); i++) {
      vector<int>& list = skillIndex[wanted[i]];
      for (size_t k = 0; k < list.size(); k++) {
        counts[list[k]] = counts[list[k]] + 1;
      }
    }

    priority_queue<ClubMatch> pq;
    for (size_t i = 0; i < clubs.size(); i++) {
      if (!clubs[i].auditionOpen) continue;
      int count = counts[(int)i];
      if (wanted.size() > 0 && count == 0) continue;
      ClubMatch m;
      m.clubIndex = (int)i;
      m.matchCount = count;
      m.clubName = clubs[i].name;
      pq.push(m);
    }

    string list = "";
    while (!pq.empty()) {
      ClubMatch m = pq.top();
      pq.pop();
      if (list != "") list += ",";
      list += auditionClubJson(m);
    }

    // Keep the last five skill searches in the deque.
    string searchText = "";
    for (size_t i = 0; i < wanted.size(); i++) {
      if (i > 0) searchText += ", ";
      searchText += wanted[i];
    }
    if (searchText != "") {
      recentSearches.push_front(searchText);
      while (recentSearches.size() > 5) recentSearches.pop_back();
    }

    string recent = "";
    for (size_t i = 0; i < recentSearches.size(); i++) {
      if (i > 0) recent += ",";
      recent += "\"" + esc(recentSearches[i]) + "\"";
    }
    return "{\"clubs\":[" + list + "],\"recent\":[" + recent + "]}";
  }

  // Every skill name, taken from the skill map keys.
  string skillsJson() {
    string out = "";
    bool first = true;
    for (map<string, vector<int> >::iterator it = skillIndex.begin(); it != skillIndex.end(); ++it) {
      if (!first) out += ",";
      out += "\"" + esc(it->first) + "\"";
      first = false;
    }
    return "{\"skills\":[" + out + "]}";
  }

  // All instagram style posts.
  string postsJson() {
    string out = "";
    for (size_t i = 0; i < posts.size(); i++) {
      if (i > 0) out += ",";
      stringstream j;
      j << "{\"handle\":\"" << esc(posts[i].handle) << "\",";
      j << "\"caption\":\"" << esc(posts[i].caption) << "\",";
      j << "\"date\":\"" << esc(posts[i].date) << "\"}";
      out += j.str();
    }
    return "{\"posts\":[" + out + "]}";
  }

  // All sponsors.
  string sponsorsJson() {
    string out = "";
    for (size_t i = 0; i < sponsors.size(); i++) {
      if (i > 0) out += ",";
      out += sponsorJson((int)i);
    }
    return "{\"sponsors\":[" + out + "]}";
  }

  // Adds one interest request to the queue and works it first in first out.
  string interestJson(const string& body) {
    string clubName = formValue(body, "club");
    string student = formValue(body, "student");
    if (clubName == "" || student == "") {
      return "{\"ok\":false,\"message\":\"Please send both a club name and a student name.\"}";
    }
    interestQueue.push("student " + student + " wants to join " + clubName);
    while (!interestQueue.empty()) {
      string entry = interestQueue.front();
      interestQueue.pop();
      cout << "Interest processed: " << entry << endl;
    }
    string message = "Thanks " + student + ". Your interest in " + clubName + " is noted.";
    return "{\"ok\":true,\"message\":\"" + esc(message) + "\"}";
  }

  // Builds a response that sends one site file from this folder.
  Response fileResponse(const string& name, const string& type) {
    Response res;
    res.type = type;
    res.body = readFile(name);
    if (res.body == "") {
      res.code = 404;
      res.type = "application/json";
      res.body = "{\"error\":\"file missing\"}";
    }
    return res;
  }

  // Picks the right response for one request.
  Response route(const string& method, const string& path, const string& query,
                 const string& body) {
    Response res;
    if (method == "GET" && (path == "/" || path == "/index.html")) {
      return fileResponse("index.html", "text/html; charset=utf-8");
    }
    if (method == "GET" && path == "/style.css") {
      return fileResponse("style.css", "text/css; charset=utf-8");
    }
    if (method == "GET" && path == "/script.js") {
      return fileResponse("script.js", "application/javascript; charset=utf-8");
    }
    if (method == "GET" && path == "/api/home") {
      res.body = homeJson();
      return res;
    }
    if (method == "GET" && path == "/api/events") {
      res.body = eventsJson(formValue(query, "status"));
      return res;
    }
    if (method == "GET" && path == "/api/clubs") {
      res.body = clubsJson();
      return res;
    }
    if (method == "GET" && path == "/api/auditions") {
      res.body = auditionsJson(formValue(query, "skills"));
      return res;
    }
    if (method == "GET" && path == "/api/skills") {
      res.body = skillsJson();
      return res;
    }
    if (method == "GET" && path == "/api/posts") {
      res.body = postsJson();
      return res;
    }
    if (method == "GET" && path == "/api/sponsors") {
      res.body = sponsorsJson();
      return res;
    }
    if (method == "POST" && path == "/api/interest") {
      res.body = interestJson(body);
      return res;
    }
    res.code = 404;
    res.body = "{\"error\":\"not found\"}";
    return res;
  }

  // Reads one request and sends one response on this client socket.
  void handleClient(SocketType client) {
    string data = "";
    char buf[4096];
    size_t headEnd = string::npos;
    while (true) {
      int got = recv(client, buf, sizeof(buf), 0);
      if (got <= 0) break;
      data.append(buf, got);
      headEnd = data.find("\r\n\r\n");
      if (headEnd != string::npos) {
        string headers = data.substr(0, headEnd);
        int need = 0;
        string low = lower(headers);
        size_t pos = low.find("content-length:");
        if (pos != string::npos) {
          stringstream ss(headers.substr(pos + 15));
          ss >> need;
        }
        size_t have = data.size() - (headEnd + 4);
        if ((int)have >= need) break;
      }
      if (data.size() > 200000) break;
    }

    string method = "";
    string target = "";
    size_t first = data.find(' ');
    if (first != string::npos) {
      method = data.substr(0, first);
      size_t second = data.find(' ', first + 1);
      if (second != string::npos) {
        target = data.substr(first + 1, second - first - 1);
      }
    }

    string path = target;
    string query = "";
    string body = "";
    size_t q = target.find('?');
    if (q != string::npos) {
      path = target.substr(0, q);
      query = target.substr(q + 1);
    }
    if (headEnd != string::npos) {
      body = data.substr(headEnd + 4);
    }

    Response res;
    if (method == "" || path == "") {
      res.code = 400;
      res.type = "application/json";
      res.body = "{\"error\":\"bad request\"}";
    } else {
      res = route(method, path, query, body);
    }

    cout << method << " " << path << " -> " << res.code << endl;

    string statusText = "200 OK";
    if (res.code == 400) statusText = "400 Bad Request";
    if (res.code == 404) statusText = "404 Not Found";

    string out = "HTTP/1.1 " + statusText + "\r\n";
    out += "Content-Type: " + res.type + "\r\n";
    out += "Content-Length: " + numText((int)res.body.size()) + "\r\n";
    out += "Connection: close\r\n\r\n";
    out += res.body;

    int flags = 0;
#ifdef MSG_NOSIGNAL
    flags = MSG_NOSIGNAL;
#endif
    size_t sent = 0;
    while (sent < out.size()) {
      int n = send(client, out.c_str() + sent, (int)(out.size() - sent), flags);
      if (n <= 0) break;
      sent += (size_t)n;
    }
  }

  // Opens the port and answers one request at a time.
  void run() {
    SocketType serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (socketBad(serverSocket)) {
      cout << "Could not create the socket" << endl;
      return;
    }
    int yes = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));
    sockaddr_in addr = sockaddr_in();
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(serverSocket, (sockaddr*)&addr, sizeof(addr)) < 0) {
      cout << "Could not bind port 8080" << endl;
      closeSocket(serverSocket);
      return;
    }
    listen(serverSocket, 8);
    cout << "JUIT Club Hub is running at http://localhost:8080" << endl;
    while (true) {
      SockLen len = sizeof(addr);
      SocketType client = accept(serverSocket, (sockaddr*)&addr, &len);
      if (socketBad(client)) continue;
      handleClient(client);
      closeSocket(client);
    }
  }
};

// Starts the server.
int main() {
#ifdef _WIN32
  WSADATA info;
  WSAStartup(MAKEWORD(2, 2), &info);
#endif
  Server server;
  server.addSampleData();
  server.buildIndexes();
  server.checkVenues();
  server.run();
  return 0;
}
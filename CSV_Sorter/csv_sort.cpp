#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <algorithm>
#include <fstream>
#include <memory>
#include <regex>
#include <chrono>
#include <execution>

using namespace std;
using namespace std::chrono;

#define SIZE 80000000
char buf[SIZE];

class Date {
private:
	int day;
	int month;
	int year;
public:
	Date(string);
	int getYear() const { return year; }
	int getMonth() const { return month; }
	int getDay() const { return day; }
};

template <typename func>
milliseconds TimeMe(func f) {
	time_point<std::chrono::system_clock> start, end;
	start = std::chrono::system_clock::now();
	f();
	end = std::chrono::system_clock::now();
	return duration_cast<milliseconds> (end - start);

}

Date::Date(string in) {
	string temp = in.substr(0, 2);
	day = stoi(temp);
	temp = in.substr(3, 2);
	month = stoi(temp);
	temp = in.substr(6, 4);
	year = stoi(temp);
}

class does_not_exist : public exception {
public:
	const char* what() const throw () {
		return "The column does not exist!";
	}
};

class CSV_Line {
private:
	string_view csv_line;
	variant<int, double, string, Date> sortfield;

public:
	friend class CSV_File;
	enum class dataType { int_type = 0, double_type = 1, date_type = 2, string_type = 3 };
	CSV_Line(int, string_view&, dataType);
	friend bool operator >(const Date& d1, const Date& d2);
	friend bool operator <(const Date& d1, const Date& d2);
};

bool operator < (const Date& d1, const Date& d2) {
	if (d1.getYear() < d2.getYear())
		return true;
	else if (d1.getYear() > d2.getYear())
		return false;
	if (d1.getMonth() < d2.getMonth())
		return true;
	else if (d1.getMonth() > d2.getMonth())
		return false;
	if (d1.getDay() < d2.getDay())
		return true;
	else if (d1.getDay() > d2.getDay())
		return false;
	else return true;
}

bool operator > (const Date& d1, const Date& d2) {
	if (d1.getYear() > d2.getYear())
		return true;
	else if (d1.getYear() < d2.getYear())
		return false;
	if (d1.getMonth() > d2.getMonth())
		return true;
	else if (d1.getMonth() < d2.getMonth())
		return false;
	if (d1.getDay() > d2.getDay())
		return true;
	else if (d1.getDay() < d2.getDay())
		return false;
	else return true;
}

CSV_Line::CSV_Line(int col, string_view& line, dataType fieldType) : csv_line(line) {
	size_t start = 0, end = string_view::npos;
	string temp;
	for (int i = 0; i < col; ++i, ++start)
		start = line.find(",", start);
	end = line.find(",", start + 1);
	if (end == string_view::npos)
		temp = static_cast<string>(line.substr(start));
	else
		temp = static_cast<string>(line.substr(start, end - start));

	if (fieldType == dataType::int_type)
		sortfield = stoi(temp);
	else if (fieldType == dataType::double_type)
		sortfield = stod(temp);
	else if (fieldType == dataType::date_type)
		sortfield = Date(temp);
	else
		sortfield = temp;
}

class CSV_File {
private:
	fstream inFile;
	fstream outFile;
	string infname;
	string outfname;
	unique_ptr<char[]> buffer;
	string_view svbuf;
	string header;
	string sortField;
	vector<CSV_Line> lines;
	vector <string> rg = { "^(\\+|-)?\\d+", "^(\\+|-)?\\d+\\.\\d+", "^[0-2][0-9]\\/[0-9][0-9]\\/\\d{4}"};
	char order;
	
	string_view& getcsvLine(string_view&);
	int colToSort();
	CSV_Line::dataType typeDetector(int, string_view& line);

public:
	CSV_File(string, string, string, char);
	void readFile();
	void processFile();
	void sortFile();
	void writeFile();
};

CSV_File::CSV_File(string infname, string outfname, string sortField, char order) : infname(infname), outfname(outfname), sortField(sortField), order(order) {};

string_view& CSV_File::getcsvLine(string_view& line) {
	auto lineend = svbuf.find("\n");
	if (lineend == string_view::npos) {
		svbuf = "";
		return line;
	}
	line = svbuf.substr(0, lineend);
	svbuf.remove_prefix(lineend + 1);
	return line;
}

int CSV_File::colToSort() {
	int col = 0;
	size_t start = 0;
	bool exists = false;
	try {
		while (true) {
			auto end = header.find(",", start);
			if (sortField == header.substr(start, end - start)) {
				exists = true;
				break;
			}
			else if (end == string_view::npos)
				break;
			else {
				col++;
				start = end + 1;
			}
		}
		if (exists == false)
			throw does_not_exist();
	}
	catch (does_not_exist & e) {
		cout << "Exception:" << endl;
		cout << e.what() << endl;
		exit(1);
	}
	return col;
}

CSV_Line::dataType CSV_File::typeDetector(int col, string_view& line) {
	regex i(rg[0]), doub(rg[1]), dat(rg[2]);
	size_t start = 0;
	for (int i = 0; i < col; ++i, ++start)
		start = line.find(",", start);
	auto end = line.find(",", start + 1);
	string temp;

	if (end == string_view::npos)
		temp = line.substr(start);
	else
		temp = line.substr(start, end - start);
		
	if (regex_match(temp, i))
		return CSV_Line::dataType::int_type;
	else if (regex_match(temp, doub))
		return CSV_Line::dataType::double_type;
	else if (regex_match(temp, dat))
		return CSV_Line::dataType::date_type;
	else
		return CSV_Line::dataType::string_type;
}

void CSV_File::readFile() {
	inFile.open(infname, ios::in);
	if (!inFile.is_open()) {
		cout << "Error opening file";
		exit(1);
	}
	
	inFile.seekg(0, inFile.end);
	const auto length = static_cast<uint64_t>(inFile.tellg()) + 1;
	inFile.seekg(0, inFile.beg);
	buffer = make_unique<char[]>(length);
	memset(buffer.get(), 0, length);
	inFile.read(buffer.get(), length);
	svbuf = buffer.get();
	inFile.close();

	return;
}

void CSV_File::processFile() {
	string_view line = getcsvLine(line);
	header = static_cast<string>(line);
	int col = colToSort();

	line = getcsvLine(line);
	CSV_Line::dataType t = typeDetector(col, line);
	lines.emplace_back(CSV_Line(col, line, t));

	while (!svbuf.empty()) {
		line = getcsvLine(line);
		lines.emplace_back(CSV_Line(col, line, t));
	}

	return;
}

void CSV_File::sortFile() {
	if(order == 'a')
		sort(std::execution::par_unseq, lines.begin(), lines.end(), [](const CSV_Line& s1, const CSV_Line& s2) {return s1.sortfield < s2.sortfield; });
	if (order == 'd')
		sort(execution::par_unseq, lines.rbegin(), lines.rend(), [](const CSV_Line& s1, const CSV_Line& s2) {return s1.sortfield < s2.sortfield; });
	return;
}

void CSV_File::writeFile() {
	outFile.open(outfname, ios::out);
	outFile.rdbuf()->pubsetbuf(buf, SIZE);
	outFile << header << endl;
	for (auto i = lines.begin(); i != lines.end(); ++i)
		outFile << (*i).csv_line << '\n';
	outFile.close();
	return;
}

int main(int argc, char* argv[]) {
	if (argc == 5) {
		string ifname = argv[1];
		string ofname = argv[2];
		string sortField = argv[3];
		string order = argv[4];
		CSV_File File(ifname, ofname, sortField, order[1]);
		milliseconds m;
		
		m = TimeMe([&File]() {File.readFile(); });
		cout << "Read File Execution Time: " << m.count() << " milliseconds" << endl;
		
		m = TimeMe([&File]() {File.processFile(); });
		cout << "Process File Execution Time: " << m.count() << " milliseconds" << endl;
		
		m  = TimeMe([&File]() {File.sortFile(); });
		cout << "Sort File Execution Time: " << m.count() << " milliseconds" << endl;
		
		m = TimeMe([&File]() {File.writeFile(); });
		cout << "Write File Execution Time: " << m.count() << " milliseconds" << endl;
	}
	
	else {
		cout << "Incorrect Use! " << endl;
		cout << "Usage: " << "infilePath outfilePath sortField -a/d" << endl;
		return -1;
	}

	return 0;
}
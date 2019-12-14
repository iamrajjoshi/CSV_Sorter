#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <algorithm>
#include <fstream>
#include <memory>
#include <regex>

using namespace std;

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
	string_view line;
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

CSV_Line::CSV_Line(int col, string_view& line, dataType fieldType) : line(line) {
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
	fstream file;
	string fname;
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
	CSV_File(string, string, char);
	void readFile();
	void sortFile();
	void writeFile();
	
};

CSV_File::CSV_File(string fname, string sortField, char order) : fname(fname), sortField(sortField), order(order) {};

string_view& CSV_File::getcsvLine(string_view& line) {
	auto lineend = svbuf.find("\n");
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
	file.open(fname, ios::in);
	if (!file.is_open()) {
		cout << "Error opening file";
		exit(1);
	}
	
	file.seekg(0, file.end);
	const auto length = static_cast<uint64_t>(file.tellg()) + 1;
	file.seekg(0, file.beg);
	buffer = make_unique<char[]>(length);
	memset(buffer.get(), 0, length);
	file.read(buffer.get(), length);
	svbuf = buffer.get();
	file.close();
	return;
}

void CSV_File::sortFile() {
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

	if(order == 'a')
		sort(lines.begin(), lines.end(), [](const CSV_Line& s1, const CSV_Line& s2) {return s1.sortfield < s2.sortfield; });
	if (order == 'd')
		sort(lines.rbegin(), lines.rend(), [](const CSV_Line& s1, const CSV_Line& s2) {return s1.sortfield < s2.sortfield; });
	return;
}

void CSV_File::writeFile() {
	file.open(fname, ios::out);
	file << header << endl;
	for (auto i = lines.begin(); i != lines.end(); ++i)
		file << (*i).line << endl;
	file.close();
	return;
}

int main(int argc, char* argv[]) {
	if (argc == 4) {
		string fname = argv[1];
		string sortField = argv[2];
		string order = argv[3];
		CSV_File File(fname, sortField, order[1]);
		File.readFile();
		File.sortFile();
		File.writeFile();
	}
	else {
		cout << "Incorrect Use! " << endl;
		cout << "Usage: " << "filePath sortField -a/d" << endl;
		return -1;
	}
	return 0;
}
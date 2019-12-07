#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <algorithm>
#include <fstream>
#include <memory>

using namespace std;

class CSV_Line {
private:
	string_view line;
	variant<int, double, string> sortfield;

public:
	friend class CSV_File;
	enum class dataType { int_type = 0, double_type = 1, string_type = 2 };
	CSV_Line(int, string_view&, dataType);
};

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

int CSV_File::colToSort() {// write the execption 
	int col = 0;
	size_t start = 0;
	bool exists = false;
	while (true) {
		auto end = header.find(",", start);
		if (end == string_view::npos)
			break;
		if (sortField == header.substr(start, end - start)) {
			break;
			exists = true;
		}
			
		else {
			col++;
			start = end + 1;
		}
	}
	//if(exists == false)
		
	return col;
}

CSV_Line::dataType CSV_File::typeDetector(int col, string_view& line) {
	size_t start = 0;
	for (int i = 0; i < col; ++i, ++start)
		start = line.find(",", start);
	auto end = line.find(",", start + 1);
	string temp;

	if (end == string_view::npos)
		temp = line.substr(start);
	else
		temp = line.substr(start, end - start);

	if (temp.find_first_not_of("-0123456789") == string::npos)
		return CSV_Line::dataType::int_type;
	else if (temp.find_first_not_of("-0123456789.") == string::npos)
		return CSV_Line::dataType::double_type;
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

int main() {
	char buffer_c[] = "name,age,num\nJoe,42,45.5\nFred,50,50.5\nAlbert,21,44.5\n";
	string fname = "C:/Users/geeky/MEGA/Workspace/Visual Studio 2019/CSV_Sorter/CSV_Sorter/test_small.csv";
	string sortField = "seq";
	CSV_File File(fname, sortField, 'd');
	File.readFile();
	File.sortFile();
	File.writeFile();

	return 0;
}
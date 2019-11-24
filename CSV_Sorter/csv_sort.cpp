#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <algorithm>
#include <fstream>
using namespace std;

enum class Type { int_type = 0, double_type = 1, string_type = 2 };

class CSV_Line {
public:
	string_view line;
	variant<int, double, string> sortfield;
	CSV_Line(int col, string_view& line, Type fieldType) : line(line) {
		size_t start = 0, end = string_view::npos;
		string temp;
		for (int i = 0; i < col; ++i, ++start)
			start = line.find(",", start);
		end = line.find(",", start + 1);
		if (end == string_view::npos)
			temp = static_cast<string>(line.substr(start));
		else
			temp = static_cast<string>(line.substr(start, end-start));

		if (fieldType == Type::int_type)
			sortfield = stoi(temp);
		else if (fieldType == Type::double_type)
			sortfield = stod(temp);
		else
			sortfield = temp;

	}
};

string_view& getcsvLine(string_view& csvbuf, string_view& line) {
	auto lineend = csvbuf.find("\n");
	line = csvbuf.substr(0, lineend);
	csvbuf.remove_prefix(lineend + 1);
	return line;
}

int colToSort(string& headerToSort, string_view& headers) {
	int col = 0;
	size_t start = 0;

	while (true) {
		auto end = headers.find(",", start);
		if (end == string_view::npos)
			break;
		if (headerToSort == headers.substr(start, end-start))
			break;
		else {
			col++;
			start = end + 1;
		}
	}
	return col;
}

Type typeDetector(int col, string_view& line) {
	size_t start = 0;
	for (int i = 0; i < col; ++i, ++start)
		start = line.find(",", start);
	auto end = line.find(",", start + 1);
	string temp;

	if (end == string_view::npos)
		temp = line.substr(start);
	else
		temp =line.substr(start, end -start);

	if (temp.find_first_not_of("-0123456789") == string::npos)
		return Type::int_type;
	else if (temp.find_first_not_of("-0123456789.") == string::npos)
		return Type::double_type;
	else
		return Type::string_type;
}

string readFile(string& fname) {
	fstream file;
	file.open(fname, ios::in);
	string str((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
	str = str.c_str();
	file.close();
	return str;
}

string_view& createLines(vector<CSV_Line>& lines, string& buffer, string& sortField) {
	string_view csv_view(buffer);
	string_view line = getcsvLine(csv_view, line);
	string_view csv_header = (line);
	int col = colToSort(sortField, csv_header);

	line = getcsvLine(csv_view, line);
	Type t = typeDetector(col, line);
	lines.emplace_back(CSV_Line(col, line, t));

	while (!csv_view.empty()) {
		line = getcsvLine(csv_view, line);
		lines.emplace_back(CSV_Line(col, line, t));
	}

	return csv_header;
}

void writeFile(string& fname,string_view header, vector<CSV_Line>& lines) {
	fstream file;
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
	vector<CSV_Line> lines;
	
	string buffer = readFile(fname);
	string_view header = createLines(lines, buffer, sortField);
	
	sort(lines.begin(), lines.end(), [](const CSV_Line& s1, const CSV_Line& s2) {return s1.sortfield < s2.sortfield; });
	
	writeFile(fname, header, lines);

	return 0;
}
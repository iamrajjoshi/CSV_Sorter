#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <algorithm>
using namespace std;

enum type { int_type, double_type, string_type };

class csv_line {
public:
	string_view line;
	variant<int, double, string> sortfield;
	csv_line(int col, string_view& line, type fieldType) : line(line) {
		size_t start = 0, end = string_view::npos;
		string temp;
		for (int i = 0; i < col; ++i, ++start)
			start = line.find(",", start);
		end = line.find(",", start + 1);
		if (end == string_view::npos)
			temp = static_cast<string>(line.substr(start));
		else
			temp = static_cast<string>(line.substr(start, end-start));

		if (fieldType == int_type)
			sortfield = stoi(temp);
		else if (fieldType == double_type)
			sortfield = stod(temp);
		else
			sortfield = temp;

	}
};

string_view& getsvline(string_view& csvbuf, string_view& line) {
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

type typeDetector(int col, string_view& line) {
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
		return int_type;
	else if (temp.find_first_not_of("-0123456789.") == string::npos)
		return double_type;
	else
		return string_type;
}

int main() {
	char buffer[] = "name,age,num\nJoe,42,45.5\nFred,50,50.5\nAlbert,21,44.5\n";
	vector<csv_line> lines;
	string_view line;

	string sortwith = "num";

	string_view csv_view(buffer);
	line = getsvline(csv_view, line);
	string_view csv_header = (line);
	cout << line << endl;
	int col = colToSort(sortwith, csv_header);

	line = getsvline(csv_view, line);
	type t = typeDetector(col, line);
	cout << line << endl;
	lines.emplace_back(csv_line(col, line, t));
	while (!csv_view.empty()) {
		line = getsvline(csv_view, line);
		cout << line << endl;
		lines.emplace_back(csv_line(col, line, t));
	}
	cout << endl;
	sort(lines.begin(), lines.end(), [](const csv_line& s1, const csv_line& s2) {return s1.sortfield < s2.sortfield; });
	
	for (auto i = lines.begin(); i != lines.end(); ++i)
		cout << (*i).line << endl;
	
	return 0;
}
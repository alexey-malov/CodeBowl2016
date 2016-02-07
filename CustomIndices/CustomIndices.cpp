#include <functional>
#include <sstream>
#include <iomanip>
#include <memory>
#include <unordered_map>
#include <map>
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <cmath>
#include <deque>

using namespace std;

typedef function<double()> Fn;
typedef vector<string> Quote;

// A lazy calculated value that is either a number or a function
class Value
{
public:
	Value(const Fn & fn = Fn())
		: m_fn(fn)
	{
	}

	void SetValue(double value)
	{
		m_value = value;
		m_initialized = true;
	}

	double GetValue()const
	{
		if (!m_initialized)
		{
			if (m_fn)
			{
				m_value = m_fn();
			}
			m_initialized = true;
		}
		return m_value;
	}
private:
	mutable bool m_initialized = false;
	mutable double m_value = NAN;
	Fn m_fn;
};
typedef shared_ptr<Value> ValuePtr;

struct Database
{
	unordered_map<string, ValuePtr> values;
	map<string, ValuePtr> indices;
	deque<ValuePtr> indexDependencies;

	// Indices are calculated in order of their appearance in the database
	// In this case each index depends on earlier calculated indices and quotes
	// Recursion is eliminated via memoization
	void CalcIndices()
	{
		for (auto & index : indexDependencies)
		{
			ignore = index->GetValue();
		}
	}

	ValuePtr GetValue(const string& id)const
	{
		return values.at(id);
	}

	void AddValue(const string & id, const ValuePtr &value)
	{
		values.emplace(id, value);
	}

	void AddIndex(const string & id, const ValuePtr & value)
	{
		indices.emplace(id, value);
		indexDependencies.push_back(value);
	}

	void SetValue(const string& id, double value)
	{
		values.at(id)->SetValue(value);
	}
};

typedef vector<string> Quote;

void ParseRule(const Quote &quote, Database &db)
{
	auto & id = quote.at(1);
	auto & operation = quote.at(2);

	// An index calculation function
	Fn fn;
	if (operation == "+" || operation == "-")
	{
		auto arg1 = db.GetValue(quote.at(3));
		auto arg2 = db.GetValue(quote.at(4));
		fn = (operation == "+") ? Fn([=]{ return arg1->GetValue() + arg2->GetValue(); })
			: Fn([=]{ return arg1->GetValue() - arg2->GetValue(); });
	}
	else if (operation != "S")
	{
		throw std::invalid_argument("Unknown operation");
	}
	auto value = make_shared<Value>(fn);
	db.AddValue(id, value);

	if (fn) // If the value is an index, register it
	{
		db.AddIndex(id, value);
	}
}

void ParseQuote(const Quote &quote, Database &db)
{
	if (!quote.empty())
	{
		if (quote.front() == "R")
		{
			ParseRule(quote, db);
		}
		else if (quote.front() == "Q")
		{
			db.SetValue(quote.at(1), stod(quote.at(2)));
		}
	}
}

void PrintIndices(ostream & out, const Database &db)
{
	out << setprecision(2) << fixed;
	for (auto & index : db.indices)
	{
		out << index.first << ": " << index.second->GetValue() << endl;
	}
}

void ReadDatabase(istream & in, Database & db)
{
	string line, item;
	Quote quote;
	while (getline(in, line))
	{
		if (!line.empty())
		{
			quote.clear();
			istringstream ss(line);
			while (getline(ss, item, '|'))
			{
				quote.push_back(item);
			}
			ParseQuote(quote, db);
		}
	}
}

int main()
{
	try
	{
		Database db;

		ReadDatabase(cin, db);
		db.CalcIndices();
		PrintIndices(cout, db);
	}
	catch (const exception & e)
	{
		cout << e.what() << endl;
		return 1;
	}

	return 0;
}

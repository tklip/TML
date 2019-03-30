// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.
#include <string>
#include <cstring>
#include <sstream>
#include <fstream>
#include <vector>
#include "input.h"
using namespace std;

#define dot_expected L"'.' expected.\n"
#define sep_expected L"Term or ':-' or '.' expected.\n"
#define unmatched_quotes L"Unmatched \"\n"
#define err_inrel L"Unable to read the input relation symbol.\n"
#define err_src L"Unable to read src file.\n"
#define err_dst L"Unable to read dst file.\n"
#define err_quotes L"expected \".\n"
#define err_dots L"two consecutive dots, or dot in beginning of document.\n"
#define err_quote L"' should come before and after a single character only.\n"
#define err_fname L"malformed filename.\n"
#define err_directive_arg L"invalid directive argument.\n"
#define err_escape L"invalid escaped character\n"
#define err_int L"malformed int.\n"
#define err_lex L"lexer error (please report as a bug).\n"
#define err_parse L"parser error (please report as a bug).\n"
#define err_chr L"unexpected character.\n"
#define err_body L"rule's body expected.\n"
#define err_prod L"production's body expected.\n"
#define err_term_or_dot L"term or dot expected.\n"
#define err_close_curly L"'}' expected.\n"
#define err_fnf L"file not found.\n"
#define err_rule_dir_prod_expected L"rule or production or directive expected.\n"
#define err_paren L"unbalanced parenthesis.\n"
#define err_relsym_expected L"expected relation name in beginning of term.\n"
#define err_paren_expected \
	L"expected parenthesis after a nonzero arity relation symbol.\n"

lexeme lex(pcws s) {
	while (iswspace(**s)) ++*s;
	if (!**s) return { 0, 0 };
	cws t = *s;
	if (**s == L'"') {
		while (*++*s != L'"')
			if (!**s) parse_error(unmatched_quotes, *s);
			else if (**s == L'\\' && !wcschr(L"\\\"", *++*s))
				parse_error(err_escape, *s);
		return { t, ++(*s) };
	}
	if (**s == L'<') {
		while (*++*s != L'>') if (!**s) parse_error(err_fname, *s);
		return { t, (*s)++ };
	}
	if (**s == L'\'') {
		if (*(*s + 2) != L'\'') parse_error(err_quote, *s);
		return { t, ++++++*s };
	}
	if (**s == L':') {
		if (*++*s==L'-' || **s==L'=') return ++*s, lexeme{ *s-2, *s };
		else parse_error(err_chr, *s);
	}
	if (wcschr(L"!~.,(){}@=>", **s)) return ++*s, lexeme{ *s-1, *s };
	if (wcschr(L"?-", **s)) ++*s;
	if (!iswalnum(**s)) parse_error(err_chr, *s);
	while (iswalnum(**s)) ++*s;
	return { t, *s };
}

lexemes prog_lex(cws s) {
	lexeme l;
	lexemes r;
	do { if ((l = lex(&s)) != lexeme{0, 0}) r.push_back(l); } while (*s);
	return r;
}

bool directive::parse(const lexemes& l, size_t& pos) {
	if (*l[pos][0] != '@') return false;
	if (rel = l[++pos], *l[++pos][0] == L'<') fname = true;
	else if (*l[pos][0] == L'"') fname = false;
	else parse_error(err_directive_arg, l[pos]);
	if (arg = l[pos++], *l[pos++][0] != '.')
		parse_error(dot_expected, l[pos]);
	return true;
}

int_t get_int_t(cws from, cws to) {
	int_t r = 0;
	bool neg = false;
	if (*from == L'-') neg = true, ++from;
	for (cws s = from; s != to; ++s) if (!iswdigit(*s))
		parse_error(err_int, from);
	wstring s(from, to - from);
	try { r = stoll((const char*)s.c_str()); }
	catch (...) { parse_error(err_int, from); }
	return neg ? -r : r;
}

bool elem::parse(const lexemes& l, size_t& pos) {
	if (L'(' == *l[pos][0]) return e = l[pos++], type = OPENP, true;
	if (L')' == *l[pos][0]) return e = l[pos++], type = CLOSEP, true;
	if (!iswalnum(*l[pos][0]) && !wcschr(L"'-?", *l[pos][0])) return false;
	if (e = l[pos], *l[pos][0] == L'\'') {
		if (l[pos][1]-l[pos][0]!=3||*(l[pos][1]-1)!=L'\'')
			parse_error(err_quote, l[pos]);
		type = CHR, e = { l[pos][0] + 1, l[pos][1]-1 };
	} else if (*l[pos][0] == L'?') type = VAR;
	else if (iswalpha(*l[pos][0])) type = SYM;
	else type = NUM, num = get_int_t(l[pos][0], l[pos][1]);
	return ++pos, true;
}

bool raw_term::parse(const lexemes& l, size_t& pos) {
	if ((neg = *l[pos][0] == L'~')) ++pos;
	while (!wcschr(L".:,", *l[pos][0]))
		if (e.emplace_back(), !e.back().parse(l, pos)) return false;
	if (e[0].type != elem::SYM) parse_error(err_relsym_expected, l[pos]);
	if (e.size() == 1) return calc_arity(), true;
	if (e[1].type != elem::OPENP) parse_error(err_paren_expected, l[pos]);
	if (e.back().type != elem::CLOSEP) parse_error(err_paren, l[pos]);
	return calc_arity(), true;
}

void raw_term::calc_arity() {
	size_t dep = 0;
	arity.push_back(0);
	if (e.size() == 1) return;
	for (size_t n = 2; n < e.size()-1; ++n)
		if (e[n].type == elem::OPENP) ++dep, arity.push_back(-1);
		else if (e[n].type != elem::CLOSEP) {
			if (arity.back() < 0) arity.push_back(1);
			else ++arity.back();
		} else if (!dep--) parse_error(err_paren, e[n].e);
		else arity.push_back(-2);
	if (dep) parse_error(err_paren, e[0].e);
}

bool raw_rule::parse(const lexemes& l, size_t& pos) {
	size_t curr = pos;
	if ((goal = *l[pos][0] == L'!'))
		if ((pgoal = *l[++pos][0] == L'!'))
			++pos;
	b.resize(1);
	if (!b[0].parse(l, pos)) return pos = curr, false;
	if (*l[pos][0] == '.') return ++pos, true;
	if (*l[pos][0] != ':' || l[pos][0][1] != L'-') return pos = curr, false;
	++pos;
	raw_term t;
	while (t.parse(l, pos)) {
		if (b.push_back(t), *l[pos][0] == '.') return ++pos, true;
		if (*l[pos][0] != ',') parse_error(err_term_or_dot, l[pos]);
		++pos, t.clear();
	}
	parse_error(err_body, l[pos]);
	return false;
}

bool production::parse(const lexemes& l, size_t& pos) {
	size_t curr = pos;
	elem e;
	e.parse(l, pos);
	if (*l[pos++][0] != '=' || l[pos][0][0] != L'>') return pos = curr, false;
	for (++pos, p.push_back(e);;) {
		elem e;
		if (*l[pos][0] == '.') return ++pos, true;
		if (!e.parse(l, pos)) return false;
		p.push_back(e);
	}
	parse_error(err_prod, l[pos]);
}

bool raw_prog::parse(const lexemes& l, size_t& pos) {
	while (pos < l.size() && *l[pos][0] != L'}') {
		directive x;
		raw_rule y;
		production p;
		if (x.parse(l, pos)) d.push_back(x);
		else if (y.parse(l, pos)) r.push_back(y);
		else if (p.parse(l, pos)) g.push_back(p);
		else return false;
	}
//	for (auto x : d) wcout << x.rel<<endl<<x.arg << endl;
//	wcout<<endl;
//	for (auto x : g) { for (auto y : x.p) wcout << y << endl; wcout<<endl;}
	return true;
}

raw_progs::raw_progs(FILE* f) : raw_progs(file_read_text(f)) {}

raw_progs::raw_progs(const std::wstring& s) {
	size_t pos = 0;
	lexemes l = prog_lex(wcsdup(s.c_str()));
	if (*l[pos][0] != L'{') {
		raw_prog x;
		if (!x.parse(l, pos))
			parse_error(err_rule_dir_prod_expected, l[pos]);
		p.push_back(x);
	} else do {
		raw_prog x;
		if (++pos, !x.parse(l, pos)) parse_error(err_parse, l[pos]);
		if (p.push_back(x), *l[pos++][0] != L'}')
			parse_error(err_close_curly, l[pos]);
	} while (pos < l.size());
	//for (auto x : l) wcout << x << endl;
}

wostream& operator<<(wostream& os, const lexeme& l) {
	for (cws s = l[0]; s != l[1]; ++s) os << *s;
	return os;
}

wostream& operator<<(wostream& os, const directive& d) {
	return os << L'@' << d.rel << L' ' << d.arg << L'.';
}

wostream& operator<<(wostream& os, const elem& e) {
	if (e.type == elem::CHR) return os << '\'' << *e.e[0] << '\'';
	if (e.type == elem::OPENP || e.type == elem::CLOSEP) return os<<*e.e[0];
	return e.type == elem::NUM ? os << e.num : (os << e.e);
}

wostream& operator<<(wostream& os, const production& p) {
	os << p.p[0] << L" -> ";
	for (size_t n = 1; n < p.p.size(); ++n) os << p.p[n] << L' ';
	return os << L'.';
}

wostream& operator<<(wostream& os, const raw_term& t) {
	if (t.neg) os << L'~';
	os << t.e[0];
	os << L'(';
	for (size_t ar = 0, n = 1; ar != t.arity.size();) {
		while (t.arity[ar] == -1) ++ar, os << L'(';
		while (t.e[n].type == elem::OPENP) ++n;
		for (int_t k = 0; k != t.arity[ar];)
			if ((os << t.e[n++]), ++k != t.arity[ar]) os << L' ';
		while (n < t.e.size() && t.e[n].type == elem::CLOSEP) ++n;
		++ar;
		while (ar<t.arity.size()&&t.arity[ar] == -2) ++ar, os<<L')';
	}
	return os << L')';
}

wostream& operator<<(wostream& os, const raw_rule& r) {
	if (r.goal) os << L'!';
	if (r.pgoal) os << L'!';
	os << r.b[0];
	if (r.b.size() == 1) return os << L'.';
	os << L" :- ";
	for (size_t n = 1; n < r.b.size(); ++n)
		if ((os << r.b[n]), n != r.b.size() - 1) os << L',';
	return os << L'.';
}

wostream& operator<<(wostream& os, const raw_prog& p) {
	for (auto x : p.d) os << x << endl;
	for (auto x : p.g) os << x << endl;
	for (auto x : p.r) os << x << endl;
	return os;
}

wostream& operator<<(wostream& os, const raw_progs& p) {
	if (p.p.size() == 1) os << p.p[0];
	else for (auto x : p.p) os << L'{' << endl << x << L'}' << endl;
	return os;
}

string ws2s(const wstring& s) { return string(s.begin(), s.end()); }

off_t fsize(const char *fname) {
	struct stat s;
	return stat(fname, &s) ? 0 : s.st_size;
}

off_t fsize(cws s, size_t len) { return fsize(ws2s(wstring(s, len)).c_str()); }

wstring file_read(wstring fname) {
	wifstream s(ws2s(fname));
	wstringstream ss;
	return (ss << s.rdbuf()), ss.str();
}

wstring file_read_text(FILE *f) {
	wstringstream ss;
	wchar_t buf[32], n, l, skip = 0;
	wint_t c;
	*buf = 0;
next:	for (n = l = 0; n != 31; ++n)
		if (WEOF == (c = getwc(f))) { skip = 0; break; }
		else if (c == L'#') skip = 1;
		else if (c == L'\r' || c == L'\n') skip = 0, buf[l++] = c;
		else if (!skip) buf[l++] = c;
	if (n) {
		buf[l] = 0, ss << buf;
		goto next;
	} else if (skip) goto next;
	return ss.str();
}

wstring file_read_text(wstring wfname) {
	string fname(wfname.begin(), wfname.end());
	FILE *f = fopen(fname.c_str(), "r");
	if (!f) parse_error(err_fnf, wfname);
	wstring r = file_read_text(f);
	fclose(f);
	return r;
}

void parse_error(std::wstring e, std::wstring s) {
	parse_error(e, s.c_str());
}

void parse_error(std::wstring e, cws s) {
	wcerr << e << endl;
	cws p = s;
	while (*p != L'\n') ++p;
	wstring t(s, p-s);
	wcerr << L"at: " << t << endl;
	exit(0);
}

void parse_error(std::wstring e, cws s, size_t len) {
	parse_error(e, wstring(s, len).c_str());
}

void parse_error(wstring e, lexeme l) {
	parse_error(e, wstring(l[0], l[1]-l[0]).c_str());
}

void parser_test() {
	setlocale(LC_ALL, "");
	wcout<<raw_progs(stdin);
	exit(0);
}

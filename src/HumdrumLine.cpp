//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sat Aug  8 12:24:49 PDT 2015
// Last Modified: Fri Oct 14 11:41:22 PDT 2016 Added insertion functionality
// Filename:      HumdrumLine.cpp
// URL:           https://github.com/craigsapp/humlib/blob/master/src/HumdrumLine.cpp
// Syntax:        C++11
// vim:           syntax=cpp ts=3 noexpandtab nowrap
//
// Description:   Used to store Humdrum text lines and analytic markup
//                of the line.
//

#include <sstream>
#include <algorithm>

#include "HumdrumLine.h"
#include "HumdrumFile.h"
#include "HumNum.h"
#include "Convert.h"

namespace hum {

// START_MERGE

//////////////////////////////
//
// HumdrumLine::HumdrumLine -- HumdrumLine constructor.
//

HumdrumLine::HumdrumLine(void) : string() {
	owner = NULL;
	duration = -1;
	durationFromStart = -1;
	setPrefix("!!");
}


HumdrumLine::HumdrumLine(const string& aString) : string(aString) {
	owner = NULL;
	if ((this->size() > 0) && (this->back() == 0x0d)) {
		this->resize(this->size() - 1);
	}
	duration = -1;
	durationFromStart = -1;
	setPrefix("!!");
}


HumdrumLine::HumdrumLine(const char* aString) : string(aString) {
	owner = NULL;
	if ((this->size() > 0) && (this->back() == 0x0d)) {
		this->resize(this->size() - 1);
	}
	duration = -1;
	durationFromStart = -1;
	setPrefix("!!");
}



//////////////////////////////
//
// HumdrumLine::~HumdrumLine -- HumdrumLine deconstructor.
//

HumdrumLine::~HumdrumLine() {
	// free stored HumdrumTokens:
	for (int i=0; i<(int)tokens.size(); i++) {
		if (tokens[i] != NULL) {
			delete tokens[i];
			tokens[i] = NULL;
		}
	}
}



//////////////////////////////
//
// HumdrumLine::setLineFromCsv -- Read a HumdrumLine from a CSV line.
// default value: separator = ","
//

void HumdrumLine::setLineFromCsv(const char* csv, const string& separator) {
	string temp = csv;
	setLineFromCsv(temp);
}



void HumdrumLine::setLineFromCsv(const string& csv, const string& separator) {
	if (csv.size() < 1) {
		return;
	}
	string newcsv = csv;
	if ((newcsv.size() > 0) && (newcsv.back() == 0x0d)) {
		newcsv.resize(newcsv.size() - 1);
	}
	// construct tab-delimited string
	string output;
	bool inquote = false;

	if ((newcsv.size() >= 2) && (newcsv[0] == '!') && (newcsv[1] == '!')) {
		// Global commands and reference records which do not start with a
		// quote are considered to be literal.
		*this = newcsv;
		return;
	}

	for (int i=0; i<(int)newcsv.size(); i++) {
		if ((newcsv[i] == '"') && !inquote) {
			inquote = true;
			continue;
		}
		if (inquote && (newcsv[i] == '"') && (newcsv[i+1] == '"')
				&& (i < (int)newcsv.length()-1)) {
			output += '"';
			i++;
			continue;
		}
		if (newcsv[i] == '"') {
			inquote = false;
			continue;
		}
		if ((!inquote) && (newcsv.substr(i, separator.size()) == separator)) {
			output += '\t';
			i += separator.size() - 1;
			continue;
		}
		output += newcsv[i];
	}
	string& value = *this;
	value = output;
}



//////////////////////////////
//
// HumdrumLine::clear -- Remove stored tokens.
//

void HumdrumLine::clear(void) {
	for (int i=0; i<(int)tokens.size(); i++) {
		if (tokens[i] != NULL) {
			delete tokens[i];
			tokens[i] = NULL;
		}
	}
}



//////////////////////////////
//
// HumdrumLine::equalChar -- return true if the character at the given
//     index is the given char.
//

bool HumdrumLine::equalChar(int index, char ch) const {
	if ((int)size() <= index) {
		return false;
	}
	if (index < 0) {
		return false;
	}
	if (((string)(*this))[index] == ch) {
		return true;
	} else {
		return false;
	}
}



//////////////////////////////
//
// HumdrumLine::isKernBoundaryStart -- Return true if the
//    line does not have any null tokens in **kern data which
//    refer to data tokens above the line.
//

bool HumdrumLine::isKernBoundaryStart(void) const {
	if (!isData()) {
		return false;
	}
	for (int i=0; i<getFieldCount(); i++) {
		if (!token(i)->isDataType("**kern")) {
			continue;
		}
		if (token(i)->isNull()) {
			return false;
		}
	}
	return true;
}



//////////////////////////////
//
// HumdrumLine::isKernBoundaryEnd -- Return true if the next
//    data line contains no null tokens in the **kern spines.
//    Assuming that a **kern spine split always starts with
//    a non-null token.
//

bool HumdrumLine::isKernBoundaryEnd(void) const {
	if (!isData()) {
		return false;
	}
	HTp ntok;
	for (int i=0; i<getFieldCount(); i++) {
		if (!token(i)->isDataType("**kern")) {
			continue;
		}
		ntok = token(i)->getNextToken();
		if (ntok == NULL) {
			continue;
		}
		while ((ntok != NULL) && !ntok->isData()) {
			ntok = ntok->getNextToken();
		}
		if (ntok == NULL) {
			continue;
		}
		if (ntok->isNull()) {
			return false;
		}
	}
	return true;
}



//////////////////////////////
//
// HumdrumLine::isComment -- Returns true if the first character
//   in the string is '!'. Could be local, global, or a reference record.
//

bool HumdrumLine::isComment(void) const {
	return equalChar(0, '!');
}



//////////////////////////////
//
// HumdrumLine::isCommentLocal -- Returns true if a local comment.
//

bool HumdrumLine::isCommentLocal(void) const {
	return equalChar(0, '!') && !equalChar(1, '!');
}



//////////////////////////////
//
// HumdrumLine::isCommentGlobal -- Returns true if a local comment.
//

bool HumdrumLine::isCommentGlobal(void) const {
	return equalChar(0, '!') && equalChar(1, '!');
}



//////////////////////////////
//
// HumdrumLine::isReference -- Returns true if a reference record.
//

bool HumdrumLine::isReference(void) const {

	if (this->size() < 5) {
		return false;
	}
	if (this->substr(0, 3) != "!!!") {
		return false;
	}
	if ((*this)[3] == '!') {
		return false;
	}
	int spaceloc = (int)this->find(" ");
	int tabloc = (int)this->find("\t");
	int colloc = (int)this->find(":");
	if (colloc == (int)string::npos) {
		return false;
	}
	if ((spaceloc != (int)string::npos) && (spaceloc < colloc)) {
		return false;
	}
	if ((tabloc != (int)string::npos) && (tabloc < colloc)) {
		return false;
	}
	return true;
}



//////////////////////////////
//
// HumdrumLine::getReferenceKey -- Return reference key if a reference
//     record.  Otherwise returns an empty string.
//

string HumdrumLine::getReferenceKey(void) const {
	if (this->size() < 5) {
		return "";
	}
	if (this->substr(0, 3) != "!!!") {
		return "";
	}
	if ((*this)[3] == '!') {
		return "";
	}
	int spaceloc = (int)this->find(" ");
	int tabloc = (int)this->find("\t");
	int colloc = (int)this->find(":");
	if (colloc == (int)string::npos) {
		return "";
	}
	if ((spaceloc != (int)string::npos) && (spaceloc < colloc)) {
		return "";
	}
	if ((tabloc != (int)string::npos) && (tabloc < colloc)) {
		return "";
	}
	return this->substr(3, colloc - 3);
}



//////////////////////////////
//
// HumdrumLine::getReferenceValue -- Return reference value if a reference
//     record.  Otherwise returns an empty string.
//

string HumdrumLine::getReferenceValue(void) const {
	if (this->size() < 5) {
		return "";
	}
	if (this->substr(0, 3) != "!!!") {
		return "";
	}
	if ((*this)[3] == '!') {
		return "";
	}
	int spaceloc = (int)this->find(" ");
	int tabloc = (int)this->find("\t");
	int colloc = (int)this->find(":");
	if (colloc == (int)string::npos) {
		return "";
	}
	if ((spaceloc != (int)string::npos) && (spaceloc < colloc)) {
		return "";
	}
	if ((tabloc != (int)string::npos) && (tabloc < colloc)) {
		return "";
	}
	return Convert::trimWhiteSpace(this->substr(colloc+1));
}



//////////////////////////////
//
// HumdrumLine::isExclusive -- Returns true if the first two characters
//     are "**".
//

bool HumdrumLine::isExclusive(void) const {
	return equalChar(1, '*') && equalChar(0, '*');
}



//////////////////////////////
//
// HumdrumLine::isTerminator -- Returns true if all tokens on the line
//    are terminators.
//

bool HumdrumLine::isTerminator(void) const {
	if (getTokenCount() == 0) {
		// if tokens have not been parsed, check line text
		return equalChar(1, '!') && equalChar(0, '*');
	}
	for (int i=0; i<getTokenCount(); i++) {
		if (!token(i)->isTerminator()) {
			return false;
		}
	}
	return true;
}



//////////////////////////////
//
// HumdrumLine::isInterp -- Returns true if starts with '*' character.
//

bool HumdrumLine::isInterp(void) const {
	return equalChar(0, '*');
}



//////////////////////////////
//
// HumdrumLine::isBarline -- Returns true if starts with '=' character.
//

bool HumdrumLine::isBarline(void) const {
	return equalChar(0, '=');
}



//////////////////////////////
//
// HumdrumLine::isData -- Returns true if data (but not measure).
//

bool HumdrumLine::isData(void) const {
	if (isComment() || isInterp() || isBarline() || isEmpty()) {
		return false;
	} else {
		return true;
	}
}



//////////////////////////////
//
// HumdrumLine::isAllNull -- Returns true if all tokens on the line
//    are null ("." if a data line, "*" if an interpretation line, "!"
//    if a local comment line).
//

bool HumdrumLine::isAllNull(void) const {
	if (!hasSpines()) {
		return false;
	}
	for (int i=0; i<getTokenCount(); i++) {
		if (!token(i)->isNull()) {
			return false;
		}
	}
	return true;
}



//////////////////////////////
//
// HumdrumLine::isAllRhythmicNull -- Returns true if all rhythmic
//    data-type tokens on the line are null ("." if a data line,
//    "*" if an interpretation line, "!" if a local comment line).
//

bool HumdrumLine::isAllRhythmicNull(void) const {
	if (!hasSpines()) {
		return false;
	}
	for (int i=0; i<getTokenCount(); i++) {
		if (!token(i)->hasRhythm()) {
			continue;
		}
		if (!token(i)->isNull()) {
			return false;
		}
	}
	return true;
}



//////////////////////////////
//
// HumdrumLine::setLineIndex -- Used by the HumdrumFileBase class to set the
//   index number of the line in the data storage for the file.
//

void HumdrumLine::setLineIndex(int index) {
	lineindex = index;
}



//////////////////////////////
//
// HumdrumLine::getLineIndex -- Returns the index number of the line in the
//    HumdrumFileBase storage for the lines.
//

int HumdrumLine::getLineIndex(void) const {
	return lineindex;
}



//////////////////////////////
//
// HumdrumLine::getLineNumber -- Returns the line index plus one.
//

int HumdrumLine::getLineNumber(void) const {
	return lineindex + 1;
}



//////////////////////////////
//
// HumdrumLine::getDuration -- Get the duration of the line.  The duration will
//    be negative one if rhythmic analysis in HumdrumFileStructure has not been
//    done on the owning HumdrumFile object.  Otherwise this is the duration of
//    the current line in the file.
//

HumNum HumdrumLine::getDuration(void) const {
	return duration;
}


HumNum HumdrumLine::getDuration(HumNum scale) const {
	return duration * scale;
}



//////////////////////////////
//
// HumdrumLine::getBarlineDuration -- Return the duration following a barline,
//    or the duration of the previous barline in the data.
//

HumNum HumdrumLine::getBarlineDuration(void) const {
	if (isBarline()) {
		return getDurationToBarline();
	} else {
		return getDurationFromBarline() + getDurationToBarline();
	}
}


HumNum HumdrumLine::getBarlineDuration(HumNum scale) const {
	if (isBarline()) {
		return getDurationToBarline(scale);
	} else {
		return getDurationFromBarline(scale) + getDurationToBarline(scale);
	}
}



//////////////////////////////
//
// HumdrumLine::setDurationFromStart -- Sets the duration from the start of the
//    file to the start of the current line.  This is used in rhythmic
//    analysis done in the HumdrumFileStructure class.
//

void HumdrumLine::setDurationFromStart(HumNum dur) {
	durationFromStart = dur;
}



//////////////////////////////
//
// HumdrumLine::getDurationFromStart -- Get the duration from the start of the
//    file to the start of the current line.  This will be -1 if rhythmic
//    analysis has not been done in the HumdrumFileStructure class.
//

HumNum HumdrumLine::getDurationFromStart(void) const {
	return durationFromStart;
}


HumNum HumdrumLine::getDurationFromStart(HumNum scale) const {
	return durationFromStart * scale;
}



//////////////////////////////
//
// HumdrumLine::getDurationToEnd -- Returns the duration from the start of the
//    line to the end of the HumdrumFile which owns this HumdrumLine.  The
//    rhythm of the HumdrumFile must be analyze before using this function;
//    otherwise a 0 will probably be returned.
//

HumNum HumdrumLine::getDurationToEnd(void) const {
	if (owner == NULL) {
		return 0;
	}
	return ((HumdrumFile*)owner)->getScoreDuration() -  durationFromStart;
}


HumNum HumdrumLine::getDurationToEnd(HumNum scale) const {
	if (owner == NULL) {
		return 0;
	}
	return scale * (((HumdrumFile*)owner)->getScoreDuration() -
		durationFromStart);
}



//////////////////////////////
//
// HumdrumLine::getDurationFromBarline -- Returns the duration from the start
//    of the given line to the first barline occurring before the given line.
//    Analysis of this data is found in HumdrumFileStructure::metricAnalysis.
//

HumNum HumdrumLine::getDurationFromBarline(void) const {
	return durationFromBarline;
}


HumNum HumdrumLine::getDurationFromBarline(HumNum scale) const {
	return durationFromBarline * scale;
}



//////////////////////////////
//
// HumdrumLine::getTrackStart --  Returns the starting exclusive interpretation
//    for the given spine/track.
//

HTp HumdrumLine::getTrackStart(int track) const {
	if (owner == NULL) {
		return NULL;
	} else {
		return ((HumdrumFile*)owner)->getTrackStart(track);
	}
}



//////////////////////////////
//
// HumdrumLine::setDurationFromBarline -- Time from the previous
//    barline to the current line.  This function is used in analyzeMeter in
//    the HumdrumFileStructure class.
//

void HumdrumLine::setDurationFromBarline(HumNum dur) {
	durationFromBarline = dur;
}



//////////////////////////////
//
// HumdrumLine::getDurationToBarline -- Time from the starting of the
//   current note to the next barline.
//

HumNum HumdrumLine::getDurationToBarline(void) const {
	return durationToBarline;
}


HumNum HumdrumLine::getDurationToBarline(HumNum scale) const {
	return durationToBarline * scale;
}



//////////////////////////////
//
// HumdrumLine::getBeat -- Returns the beat number for the data on the
//     current line given the input **recip representation for the duration
//     of a beat.  The beat in a measure is offset from 1 (first beat is
//     1 rather than 0).
//  Default value: beatrecip = "4".
//  Default value: beatdur   = 1.
//

HumNum HumdrumLine::getBeat(HumNum beatdur) const {
	if (beatdur.isZero()) {
		return beatdur;
	}
	HumNum beat = (getDurationFromBarline() / beatdur) + 1;
	return beat;
}


HumNum HumdrumLine::getBeat(string beatrecip) const {
	HumNum beatdur = Convert::recipToDuration(beatrecip);
	if (beatdur.isZero()) {
		return beatdur;
	}
	HumNum beat = (getDurationFromBarline() / beatdur) + 1;
	return beat;
}



//////////////////////////////
//
// HumdrumLine::setDurationToBarline -- Sets the duration from the current
//     line to the next barline in the score.  This function is used by
//     analyzeMeter in the HumdrumFileStructure class.
//

void HumdrumLine::setDurationToBarline(HumNum dur) {
	durationToBarline = dur;
}



//////////////////////////////
//
// HumdrumLine::setDuration -- Sets the duration of the line.  This is done
//   in the rhythmic analysis for the HumdurmFileStructure class.
//

void HumdrumLine::setDuration(HumNum aDur) {
	if (aDur.isNonNegative()) {
		duration = aDur;
	} else {
		duration = 0;
	}
}



//////////////////////////////
//
// HumdrumLine::hasSpines -- Returns true if the line contains spines.  This
//   means the the line is not empty or a global comment (which can include
//   reference records.
//

bool HumdrumLine::hasSpines(void) const {
	return (isEmpty() || isCommentGlobal()) ? false : true;
}



//////////////////////////////
//
// HumdrumLine::isGlobal -- Returns true if the line is a global record: either
//   and empty record, a global comment or a reference record.
//

bool HumdrumLine::isGlobal(void) const {
	return !hasSpines();
}



//////////////////////////////
//
// HumdrumLine::isManipulator -- Returns true if any tokens on the line are
//   manipulator interpretations.  Only null interpretations are allowed on
//   lines which contain manipulators, but the parser currently does not
//   enforce this rule.
//

bool HumdrumLine::isManipulator(void) const {
	for (int i=0; i<(int)tokens.size(); i++) {
		if (tokens[i]->isManipulator()) {
			return true;
		}
	}
	return false;
}



//////////////////////////////
//
// HumdrumLine::isEmpty -- Returns true if no characters on line.  A blank line
//   is technically disallowed in the classic Humdrum Toolkit programs, but it
//   is usually tolerated.  In humlib (and HumdrumExtras) empty lines with
//   no content (not even space characters) are allowed and treated as a
//   special class of line.
//

bool HumdrumLine::isEmpty(void) const {
	return (size() == 0) ? true : false;
}



//////////////////////////////
//
// HumdrumLine::getTokenCount --  Returns the number of tokens on the line.
//     This value is set by HumdrumFileBase in analyzeTokens.
//

int HumdrumLine::getTokenCount(void) const {
	return (int)tokens.size();
}



//////////////////////////////
//
// HumdrumLine::token -- Returns a reference to the given token on the line.
//    An invalid token index would be bad to give to this function as it
//    returns a reference rather than a pointer (which could be set to
//    NULL if invalid).  Perhaps this function will eventually throw an
//    error if the index is out of bounds.
//

HTp HumdrumLine::token(int index) const {
	return tokens[index];
}



//////////////////////////////
//
// HumdrumLine::getTokenString -- Returns a copy of the string component of
//     a token.  This code will return a segmentation fault if index is out of
//     range...
//

string HumdrumLine::getTokenString(int index) const {
	return (string(*tokens[index]));
}


//////////////////////////////
//
// HumdrumLine::createTokensFromLine -- Chop up a HumdrumLine string into
//     individual tokens.
//

int HumdrumLine::createTokensFromLine(void) {
	tokens.resize(0);
	HTp token;
	char ch;
	string tstring;

	if (this->size() == 0) {
		token = new HumdrumToken();
		token->setOwner(this);
		tokens.push_back(token);
	} else if (this->compare(0, 2, "!!") == 0) {
		token = new HumdrumToken(this->c_str());
		token->setOwner(this);
		tokens.push_back(token);
	} else {
		for (int i=0; i<(int)size(); i++) {
			ch = getChar(i);
			if (ch == '\t') {
				token = new HumdrumToken(tstring);
				token->setOwner(this);
				tokens.push_back(token);
				tstring.clear();
			} else {
				tstring += ch;
			}
		}
	}
	if (tstring.size() > 0) {
		token = new HumdrumToken(tstring);
		token->setOwner(this);
		tokens.push_back(token);
		tstring.clear();
	}

	return (int)tokens.size();
}



//////////////////////////////
//
// HumdrumLine::createLineFromTokens --  Re-generate a HumdrumLine string from
//    individual tokens on the line.  This function will be necessary to
//    run before printing a HumdrumFile if you have changed any tokens on the
//    line.  Otherwise, changes in the tokens will not be passed on to the
///   printing of the line.
//

void HumdrumLine::createLineFromTokens(void) {
	string& iline = *this;
	iline.clear();
	for (int i=0; i<(int)tokens.size(); i++) {
		iline += (string)(*tokens[i]);
		if (i < (int)tokens.size() - 1) {
			iline += '\t';
		}
	}
}



//////////////////////////////
//
// HumdrumLine::getTokens -- Returns an array of tokens pointers for a
//   Humdrum line.  This function should not be called on global comments,
//   reference records (which are a sub-cateogry of global comments).  This
//   is because a line's type may contain tabs which are not representing
//   token separators.  Empty lines are ok to input: the output token
//   list will contain one empty string.
//

void HumdrumLine::getTokens(vector<HTp>& list) {
	if (tokens.size() == 0) {
		createTokensFromLine();
	}
	list = tokens;
}



//////////////////////////////
//
// HumdrumLine::getChar -- Returns character at given index in string, or
//    null if out of range.
//

char HumdrumLine::getChar(int index) const {
	if (index < 0) {
		return '\0';
	}
	if (index >= (int)size()) {
		return '\0';
	}
	return (((string)(*this))[index]);
}



//////////////////////////////
//
// HumdrumLine::printSpineInfo -- Print the spine state information of
//    each token in a file.  Useful for debugging.  The spine info
//    is the track number, such as "1".  When the track splits into
//    subtracks, then there will be two subtracks: "(1)a" and "(1)b".
//    If the second of those subtracks splits again, then its subtracks
//    will be "((1)b)a" and "((1)b)b". If two different tracks merge, such
//    as "1" and "(2)a", then the spine info will be "1 (2)a".
//
// default value: out = cout
//

ostream& HumdrumLine::printSpineInfo(ostream& out) {
	if (isManipulator()) {
		out << *this;
	} else {
		for (int i=0; i<(int)tokens.size(); i++) {
			out << tokens[i]->getSpineInfo();
			if (i < (int)tokens.size() - 1) {
				out << '\t';
			}
		}
	}
	return out;
}



//////////////////////////////
//
// HumdrumLine::printDataTypeInfo -- Print the datatype of each token in
//     the file.  Useful for debugging.  The datatype prefix "**" is removed;
//     otherwise, it is given when you call HumdrumToken::getDataType().
//
// default value: out = cout
//

ostream& HumdrumLine::printDataTypeInfo(ostream& out) {
	if (isManipulator()) {
		out << *this;
	} else {
		for (int i=0; i<(int)tokens.size(); i++) {
			out << tokens[i]->getDataType().substr(2, string::npos);
			if (i < (int)tokens.size() - 1) {
				out << '\t';
			}
		}
	}
	return out;
}



//////////////////////////////
//
// HumdrumLine::analyzeTokenDurations -- Calculate the duration of
//    all tokens on a line.
//

bool HumdrumLine::analyzeTokenDurations(string& err) {
	if (!hasSpines()) {
		return !err.size();
	}
	for (int i=0; i<(int)tokens.size(); i++) {
		if (!tokens[i]->analyzeDuration(err)) {
			return !err.size();
		}
	}
	return !err.size();
}



//////////////////////////////
//
// HumdrumLine::analyzeTracks -- Calculate the subtrack info for subspines.
//   Subtracks index subspines strictly from left to right on the line.
//   Subspines can be exchanged and be represented left to right out of
//   original order.
//

bool HumdrumLine::analyzeTracks(string& err) {
	if (!hasSpines()) {
		return !err.size();
	}

	string info;
	int track;
	int maxtrack = 0;
	int i, j, k;

	for (i=0; i<(int)tokens.size(); i++) {
		info = tokens[i]->getSpineInfo();
		track = 0;
		for (j=0; j<(int)info.size(); j++) {
			if (!isdigit(info[j])) {
				continue;
			}
			track = info[j] - '0';
			for (k=j+1; k<(int)info.size(); k++) {
				if (isdigit(info[k])) {
					track = track * 10 + (info[k] - '0');
				} else {
					break;
				}
			}
			break;
		}
		if (maxtrack < track) {
			maxtrack = track;
		}
		tokens[i]->setTrack(track);
	}

	int subtrack;
	vector<int> subtracks;
	vector<int> cursub;

	subtracks.resize(maxtrack+1);
	cursub.resize(maxtrack+1);
	fill(subtracks.begin(), subtracks.end(), 0);
	fill(cursub.begin(), cursub.end(), 0);

	for (i=0; i<(int)tokens.size(); i++) {
		subtracks[tokens[i]->getTrack()]++;
	}
	for (i=0; i<(int)tokens.size(); i++) {
		track = tokens[i]->getTrack();
		subtrack = subtracks[track];
		if (subtrack > 1) {
			tokens[i]->setSubtrack(++cursub[tokens[i]->getTrack()]);
		} else {
			tokens[i]->setSubtrack(0);
		}
		tokens[i]->setSubtrackCount(subtracks[track]);
	}
	return !err.size();
}



//////////////////////////////
//
// HumdrumLine::printDurationInfo -- Print the analyzed duration of each
//     token in a file (for debugging).  If a token has an undefined
//     duration, then its duration is -1.  If a token represents
//     a grace note, then its duration is 0 (regardless of whether it
//     includes a visual duration).
// default value: out = cout
//

ostream& HumdrumLine::printDurationInfo(ostream& out) {
	if (isManipulator()) {
		out << *this;
	} else {
		for (int i=0; i<(int)tokens.size(); i++) {
			tokens[i]->getDuration().printMixedFraction(out);
			if (i < (int)tokens.size() - 1) {
				out << '\t';
			}
		}
	}
	return out;
}


//////////////////////////////
//
// HumdrumLine::printCsv -- print the line as a CSV
//    (comma separate value) line.
// default value: out = std::cout;
// default value: separator = ","
//

ostream& HumdrumLine::printCsv(ostream& out, const string& separator) {
	for (int i=0; i<getFieldCount(); i++) {
		token(i)->printCsv(out);
		if (i<getFieldCount()-1) {
			out << separator;
		}
	}
	out << endl;
	return out;
}



//////////////////////////////
//
// HumdrumLine::printXml -- Print the HumdrumLine as a XML element.
//

ostream& HumdrumLine::printXml(ostream& out, int level, const string& indent) {

	if (hasSpines()) {
		out << Convert::repeatString(indent, level) << "<frame";
		out << " n=\"" << getLineIndex() << "\"";
		out << " xml:id=\"" << getXmlId() << "\"";
		out << ">\n";
		level++;

		out << Convert::repeatString(indent, level) << "<frameInfo>\n";
		level++;

		out << Convert::repeatString(indent, level) << "<fieldCount>";
		out << getTokenCount() << "</fieldCount>\n";

		out << Convert::repeatString(indent, level);
		out << "<frameStart";
		out << Convert::getHumNumAttributes(getDurationFromStart());
		out << "/>\n";

		out << Convert::repeatString(indent, level);
		out << "<frameDuration";
		out << Convert::getHumNumAttributes(getDuration());
		out << "/>\n";

		out << Convert::repeatString(indent, level) << "<frameType>";
		if (isData()) {
			out << "data";
		} else if (isBarline()) {
			out << "barline";
		} else if (isInterpretation()) {
			out << "interpretation";
		} else if (isLocalComment()) {
			out << "local-comment";
		}
		out << "</frameType>\n";

		if (isBarline()) {
			// print the duration to the next barline or to the end of the score
			// if there is no barline at the end of the score.
			out << Convert::repeatString(indent, level);
			out << "<barlineDuration";
			out << Convert::getHumNumAttributes(getBarlineDuration());
			out << "/>\n";
		}

		bool bstart = isKernBoundaryStart();
		bool bend   = isKernBoundaryEnd();
		if (bstart || bend) {
			out << Convert::repeatString(indent, level);
			cout << "<kernBoundary";
			cout << " start=\"";
			if (bstart) {
				cout << "true";
			} else {
				cout << "false";
			}
			cout << "\"";
			cout << " end=\"";
			if (bend) {
				cout << "true";
			} else {
				cout << "false";
			}
			cout << "\"";
			cout << "/>\n";
		}

		level--;
		out << Convert::repeatString(indent, level) << "</frameInfo>\n";

		out << Convert::repeatString(indent, level) << "<fields>\n";
		level++;
		for (int i=0; i<getFieldCount(); i++) {
			token(i)->printXml(out, level, indent);
		}
		level--;
		out << Convert::repeatString(indent, level) << "</fields>\n";

		level--;
		out << Convert::repeatString(indent, level) << "</frame>\n";

	} else {
		// global comments, reference records, or blank lines print here.
		out << Convert::repeatString(indent, level) << "<metaFrame";
		out << " n=\"" << getLineIndex() << "\"";
		out << " token=\"" << Convert::encodeXml(((string)(*this))) << "\"";
		out << " xml:id=\"" << getXmlId() << "\"";
		out << ">\n";
		level++;

		out << Convert::repeatString(indent, level) << "<frameInfo>\n";
		level++;

		out << Convert::repeatString(indent, level);
		out << "<startTime";
		out << Convert::getHumNumAttributes(getDurationFromStart());
		out << "/>\n";

		out << Convert::repeatString(indent, level) << "<frameType>";
		if (isReference()) {
			out << "reference";
		} else if (isBlank()) {
			out << "empty";
		} else {
			out << "global-comment";
		}
		out << "</frameType>\n";

		if (isReference()) {
			out << Convert::repeatString(indent, level);
			string key = getReferenceKey();
			string language;
			string primaryLanguage;
			auto loc = key.find("@@");
			if (loc != string::npos) {
				language = key.substr(loc+2);
				key = key.substr(0, loc);
				primaryLanguage = "true";
			} else {
				loc = key.find("@");
				if (loc != string::npos) {
					language = key.substr(loc+1);
					key = key.substr(0, loc);
				}
			}

			out << "<referenceKey";
			if (language.size() > 0) {
				out << " language=\"" << Convert::encodeXml(language) << "\"";
			}
			if (primaryLanguage.size() > 0) {
				out << " primary=\"" << Convert::encodeXml(primaryLanguage) << "\"";
			}
			out << ">" << Convert::encodeXml(key);
			out << "</referenceKey>\n";

			out << Convert::repeatString(indent, level);
			out << "<referenceValue>" << Convert::encodeXml(getReferenceValue());
			out << "</referenceValue>\n";
		}

		level--;
		out << Convert::repeatString(indent, level) << "</frameInfo>\n";


		level--;
		out << Convert::repeatString(indent, level) << "</metaFrame>\n";
	}

	return out;
}



//////////////////////////////
//
// HumdrumLine::getXmlId -- Return a unique ID for the current line.
//

string HumdrumLine::getXmlId(const string& prefix) const {
	string output;
	if (prefix.size() > 0) {
		output = prefix;
	} else {
		output = getXmlIdPrefix();
	}
	output += "loc" + to_string(getLineIndex());
	return output;
}



//////////////////////////////
//
// HumdrumLine::getXmlIdPrefix -- Return the pre-set XML ID attribute
//     prefix from the owning HumdrumFile object.
//

string HumdrumLine::getXmlIdPrefix(void) const {
	if (owner == NULL) {
		return "";
	}
	return ((HumdrumFileBase*)owner)->getXmlIdPrefix();
}



//////////////////////////////
//
// HumdrumLine::printTrackInfo -- Print the analyzed track information.
//     The first (left-most) spine in a Humdrum file is track 1, the
//     next is track 2, etc.  The track value is shared by all subspines,
//     so there may be duplicate track numbers on a line if the spine
//     has split.  When the spine splits, a subtrack number is given
//     after a "." character in the printed output from this function.
//     Subtrack==0 means that there is only one subtrack.
//     Examples:
//         "1"  == Track 1, subtrack 1 (and there are no more subtracks)
//	        "1.1" == Track 1, subtrack 1 (and there are more subtracks)
//	        "1.2" == Track 1, subtrack 2 (and there may be more subtracks)
//	        "1.10" == Track 1, subtrack 10 (and there may be subtracks)
//     Each starting exclusive interpretation is assigned to a unique
//     track number.  When a *+ manipulator is given, the new exclusive
//     interpretation on the next line is give the next higher track
//     number.
//
// default value: out = cout
//

ostream& HumdrumLine::printTrackInfo(ostream& out) {
	if (isManipulator()) {
		out << *this;
	} else {
		for (int i=0; i<(int)tokens.size(); i++) {
			out << tokens[i]->getTrackString();
			if (i < (int)tokens.size() - 1) {
				out << '\t';
			}
		}
	}
	return out;
}



//////////////////////////////
//
// HumdrumLine::setOwner -- store a pointer to the HumdrumFile which
//    manages (owns) this object.
//

void HumdrumLine::setOwner(void* hfile) {
	owner = hfile;
}



//////////////////////////////
//
// HumdrumLine::getOwner -- Return the HumdrumFile which manages
//   (owns) this line.
//

HumdrumFile* HumdrumLine::getOwner(void) {
	return (HumdrumFile*)owner;
}



//////////////////////////////
//
// HumdrumLine::setParameters -- Takes a global comment with
//     the structure:
//        !!NS1:NS2:key1=value1:key2=value2:key3=value3
//     and stores it in the HumHash parent class of the line.
//

void HumdrumLine::setParameters(HumdrumLine* pLine) {
	HumdrumLine& pl = *pLine;
	if (pl.size() <= 2) {
		return;
	}
	string pdata = pLine->substr(2, pl.size()-2);
	setParameters(pdata);
}


void HumdrumLine::setParameters(const string& pdata) {
	vector<string> pieces = Convert::splitString(pdata, ':');
	if (pieces.size() < 3) {
		return;
	}
	string ns1 = pieces[0];
	string ns2 = pieces[1];
	string key;
	string value;
	int loc;
	for (int i=2; i<(int)pieces.size(); i++) {
		Convert::replaceOccurrences(pieces[i], "&colon;", ":");
		loc = (int)pieces[i].find("=");
		if (loc != (int)string::npos) {
			key   = pieces[i].substr(0, loc);
			value = pieces[i].substr(loc+1, pieces[i].size());
		} else {
			key   = pieces[i];
			value = "true";
		}
		setValue(ns1, ns2, key, value);
	}
}



//////////////////////////////
//
// HumdrumLine::appendToken -- add a token at the end of the current
//      list of tokens in the line.
//

void HumdrumLine::appendToken(HTp token) {
	// deletion will be handled by class.
	tokens.push_back(token);
}


void HumdrumLine::appendToken(const HumdrumToken& token) {
	HTp newtok = new HumdrumToken(token);
	tokens.push_back(newtok);
}


void HumdrumLine::appendToken(const string& token) {
	HTp newtok = new HumdrumToken(token);
	tokens.push_back(newtok);
}


void HumdrumLine::appendToken(const char* token) {
	HTp newtok = new HumdrumToken(token);
	tokens.push_back(newtok);
}



//////////////////////////////
//
// HumdrumLine::getKernNoteAttacks -- Return the number of kern notes
//    that attack on a line.
//

int HumdrumLine::getKernNoteAttacks(void) {
	int output = 0;
	for (int i=0; i<getFieldCount(); i++) {
		if (!token(i)->isKern()) {
			continue;
		}
		if (token(i)->isNoteAttack()) {
			output++;
		}
	}
	return output;
}



//////////////////////////////
//
// HumdrumLine::insertToken -- add a token at the end of the current
//      list of tokens in the line.
//

void HumdrumLine::insertToken(int index, HTp token) {
	// deletion will be handled by class.
	tokens.insert(tokens.begin() + index, token);
}


void HumdrumLine::insertToken(int index, const HumdrumToken& token) {
	HTp newtok = new HumdrumToken(token);
	tokens.insert(tokens.begin() + index, newtok);
}


void HumdrumLine::insertToken(int index, const string& token) {
	HTp newtok = new HumdrumToken(token);
	tokens.insert(tokens.begin() + index, newtok);
}


void HumdrumLine::insertToken(int index, const char* token) {
	HTp newtok = new HumdrumToken(token);
	tokens.insert(tokens.begin() + index, newtok);
}



//////////////////////////////
//
// operator<< -- Print a HumdrumLine. Needed to avoid interaction with
//     HumHash parent class.
//

ostream& operator<<(ostream& out, HumdrumLine& line) {
	out << (string)line;
	return out;
}

ostream& operator<< (ostream& out, HumdrumLine* line) {
	out << (string)(*line);
	return out;
}


// END_MERGE

} // end namespace hum




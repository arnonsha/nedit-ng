
#ifndef HIGHLIGHT_PATTERN_H_
#define HIGHLIGHT_PATTERN_H_

#include <string>
#include "nullable_string.h"

/* Pattern specification structure */
struct highlightPattern {
public:
	highlightPattern();
	highlightPattern(const highlightPattern &other);
	highlightPattern &operator=(const highlightPattern &rhs);
	~highlightPattern();
	
public:
	void swap(highlightPattern &other);

public:
	nullable_string name;
	char *startRE;
	char *endRE;
	char *errorRE;
	char *style;
	char *subPatternOf;
	int flags;
};


#endif

#ifndef ANALYZER_H
#define ANALYZER_H

#include <string>
#include "Events/Event.h"

class Point;
class Request;

class Analyzer {
	public:
		std::string name;
		int priority;
		std::string command;
		bool expired;
	
		Analyzer(std::string name);
		virtual ~Analyzer();

		virtual void analyze();
		virtual void complete();
		virtual void fail();
		virtual void init();
		virtual void parseMessages(const std::string &messages);
		virtual bool request(const Request &request);
		virtual void handle(event::Event *event);
};
#endif

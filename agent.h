#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include "board.h"
#include "action.h"

class agent {
public:
	agent(const std::string& args = "") {
		std::stringstream ss("name=unknown role=unknown " + args);
		for (std::string pair; ss >> pair; ) {
			std::string key = pair.substr(0, pair.find('='));
			std::string value = pair.substr(pair.find('=') + 1);
			meta[key] = { value };
		}
	}
	virtual ~agent() {}
	virtual void open_episode(const std::string& flag = "") {}
	virtual void close_episode(const std::string& flag = "") {}
	virtual action take_action(const board& b) { return action(); }
	virtual bool check_for_win(const board& b) { return false; }

public:
	virtual std::string property(const std::string& key) const { return meta.at(key); }
	virtual void notify(const std::string& msg) { meta[msg.substr(0, msg.find('='))] = { msg.substr(msg.find('=') + 1) }; }
	virtual std::string name() const { return property("name"); }
	virtual std::string role() const { return property("role"); }

protected:
	typedef std::string key;
	struct value {
		std::string value;
		operator std::string() const { return value; }
		template<typename numeric, typename = typename std::enable_if<std::is_arithmetic<numeric>::value, numeric>::type>
		operator numeric() const { return numeric(std::stod(value)); }
	};
	std::map<key, value> meta;
};

class random_agent : public agent {
public:
	random_agent(const std::string& args = "") : agent(args) {
		if (meta.find("seed") != meta.end())
			engine.seed(int(meta["seed"]));
	}
	virtual ~random_agent() {}

protected:
	std::default_random_engine engine;
};

/**
 * random environment
 * add a new random tile to an empty cell
 */
int pre_slide;
 
class rndenv : public random_agent {
public:
	rndenv(const std::string& args = "") : random_agent("name=random role=environment " + args),
		space({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }), popup(0, 9) {}

	virtual action take_action(const board& after) {
		if(pre_slide == -1){
			std::shuffle(space.begin(), space.end(), engine);
			for (int pos : space) {
				if (after(pos) != 0) continue;
				board::cell tile = bag[order[current]];
				current++;
				if(current == 3)	reset();
				return action::place(pos, tile);
			}
		}
		else{
			switch(pre_slide){
				case 0:	//up
					opposite = {12, 13, 14, 15};
					break;
				case 1:	//right
					opposite = {0, 4, 8, 12};
					break;
				case 2:	//down
					opposite = {0, 1, 2, 3};
					break;
				case 3:	//left
					opposite = {3, 7, 11, 15};
					break;
			}
			std::shuffle(opposite.begin(), opposite.end(), engine);
			for (int pos : opposite) { 
				if (after(pos) != 0) continue;
				board::cell tile = bag[order[current]];
				current++;
				if(current == 3)	reset();
				return action::place(pos, tile);
			}
		}
		return action();
	}
	void reset(){
		int i;
		for(i = 0;i < 3;i++)	{bag[i] = i + 1;	order[i] = i;}
		std::shuffle(order.begin(), order.end(), engine);
		current = 0;
	}

protected:
	std::array<int, 3> bag;
	std::array<int, 3> order;
	std::array<int, 4> opposite;
	std::array<int, 16> space;
	std::uniform_int_distribution<int> popup;
	int current;
};
 
/*class rndenv : public random_agent {
public:
	rndenv(const std::string& args = "") : random_agent("name=random role=environment " + args),
		space({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 }), popup(0, 9) {}

	virtual action take_action(const board& after) {
		std::shuffle(space.begin(), space.end(), engine);
		for (int pos : space) {
			if (after(pos) != 0) continue;
			board::cell tile = popup(engine) ? 1 : 2;
			return action::place(pos, tile);
		}
		return action();
	}

private:
	std::array<int, 16> space;
	std::uniform_int_distribution<int> popup;
};*/ 

/**
 * dummy player
 * select a legal action randomly
 */
class player : public random_agent {
public:
	player(const std::string& args = "") : random_agent("name=dummy role=player " + args),
		opcode({ 2, 3, 1, 0 }) {}

	virtual action take_action(const board& before) {
		for (int op : opcode) {
			board::reward reward = board(before).slide(op);
			pre_slide = op;
			if (reward != -1) return action::slide(op);
		}
		return action();
	}
	void reset(){
		pre_slide = -1;
	}

private:
	std::array<int, 4> opcode;
};

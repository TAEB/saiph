#include "Debug.h"
#include "EventBus.h"
#include "Inventory.h"
#include "World.h"
#include "Actions/ListInventory.h"
#include "Data/Armor.h"
#include "Data/Amulet.h"

using namespace event;
using namespace std;

/* define static variables */
const Item Inventory::NO_ITEM;
bool Inventory::_updated = false;
map<unsigned char, Item> Inventory::_items;
unsigned char Inventory::_slots[] = {'\0'};
ChangedInventoryItems Inventory::_changed;
set<unsigned char> Inventory::_lost;
unsigned long long int Inventory::_extrinsics_from_items = 0;
bool Inventory::_extrinsics_updated = false;

/* methods */
void Inventory::analyze() {
}

void Inventory::parseMessages(const string& messages) {
	if (World::lastActionID() == action::ListInventory::ID && World::menu() && messages.find(" - ") != string::npos && messages.find(" -  ") == string::npos) {
		/* last action was list inventory and we got a menu with " - " and it's not enhance menu (that got " -   "), probably listing inventory */
		string::size_type pos = 0;
		string::size_type pos2 = -1;
		if (World::curPage() == 1) {
			/* listing first page, clear changed and lost and add every inventory item to "lost" */
			_changed.clear();
			_lost.clear();
			for (map<unsigned char, Item>::iterator i = _items.begin(); i != _items.end(); ++i)
				_lost.insert(i->first);
		}
		while ((pos = messages.find(" - ", pos + 1)) != string::npos && pos > 2 && messages[pos - 3] == ' ' && messages[pos - 2] == ' ' && (pos2 = messages.find("  ", pos + 3)) != string::npos) {
			/* check that item match inventory item */
			Item item(messages.substr(pos + 3, pos2 - pos - 3));
			if (item.count() <= 0) {
				Debug::inventory() << "Failed parsing \"" << messages.substr(pos - 2, pos2 - pos + 2) << "\" as an item" << endl;
				continue;
			}
			map<unsigned char, Item>::iterator i = _items.find(messages[pos - 1]);
			if (i == _items.end()) {
				/* item is not in our inventory */
				addItem(messages[pos - 1], item);
				_changed.add(messages[pos - 1]);
			} else if (item != i->second) {
				/* item does not match item in inventory */
				unsigned char key = i->first;
				removeItem(key, i->second);
				addItem(key, item);
				_changed.add(key);
			}
			/* we (still) got this item, so it's not lost. remove it from lost */
			_lost.erase(messages[pos - 1]);
		}
		if (World::curPage() == World::maxPage()) {
			/* listing last page, add lost items to changed and remove them from inventory */
			for (set<unsigned char>::iterator l = _lost.begin(); l != _lost.end(); ++l) {
				_changed.add(*l);
				_items.erase(*l);
			}
			/* mark inventory as updated */
			_updated = true;
		}
		if (_changed.keys().size() > 0) {
			/* broadcast ChangedInventoryItems */
			EventBus::broadcast(static_cast<Event*> (&_changed));
			_extrinsics_updated = false;
		}
	} else if (messages.find(MESSAGE_NOT_CARRYING_ANYTHING) != string::npos || messages.find(MESSAGE_NOT_CARRYING_ANYTHING_EXCEPT_GOLD) != string::npos) {
		/* we're not carrying anything */
		_extrinsics_updated = true;
		_changed.clear();
		for (map<unsigned char, Item>::iterator i = _items.begin(); i != _items.end(); ++i)
			_changed.add(i->first);
		_items.clear();
		_updated = true;
		if (_changed.keys().size() > 0) {
			/* broadcast ChangedInventoryItems */
			EventBus::broadcast(static_cast<Event*> (&_changed));
		}
	} else if (messages.find(MESSAGE_STEALS) != string::npos || messages.find(MESSAGE_STOLE) != string::npos || messages.find(MESSAGE_DESTROY_POTION_FIRE) != string::npos || messages.find(MESSAGE_DESTROY_POTION_FIRE2) != string::npos || messages.find(MESSAGE_DESTROY_POTION_COLD) != string::npos || messages.find(MESSAGE_DESTROY_POTION_COLD2) != string::npos || messages.find(MESSAGE_DESTROY_RING) != string::npos || messages.find(MESSAGE_DESTROY_RING2) != string::npos || messages.find(MESSAGE_DESTROY_WAND) != string::npos || messages.find(MESSAGE_DESTROY_WAND2) != string::npos || messages.find(MESSAGE_POLYMORPH) != string::npos || messages.find(MESSAGE_FOOCUBUS_QUESTION) != string::npos || messages.find(MESSAGE_FOOCUBUS_REMOVE) != string::npos) {
		/* we got robbed, some of our stuff was destroyed. we polymorphed or encountered a foocubi.
		 * mark inventory as not updated */
		update();
	}
}

std::map<unsigned char, Item>& Inventory::items() {
	return _items;
}

const Item& Inventory::itemAtKey(const unsigned char& key) {
	map<unsigned char, Item>::iterator i = _items.find(key);
	if (i != _items.end())
		return i->second;
	return NO_ITEM;
}

const Item& Inventory::itemInSlot(const int& slot) {
	if (slot < 0 || slot >= SLOTS)
		return NO_ITEM;
	return itemAtKey(_slots[slot]);
}

const unsigned char& Inventory::keyForSlot(const int& slot) {
	if (slot < 0 || slot >= SLOTS)
		return _slots[INVALID_SLOT];
	return _slots[slot];
}

void Inventory::addItem(const unsigned char& key, const Item& item) {
	if (item.count() <= 0)
		return;
	Debug::inventory() << "Adding " << item << " to inventory slot " << key << endl;
	_extrinsics_updated = false;
	map<unsigned char, Item>::iterator i = _items.find(key);
	if (i != _items.end()) {
		/* existing item, add amount */
		i->second.count(i->second.count() + item.count());
	} else {
		/* new item */
		_items[key] = item;
	}
	/* update slot */
	setSlot(key, item);
}

void Inventory::removeItem(const unsigned char& key, const Item& item) {
	if (item.count() <= 0)
		return;
	map<unsigned char, Item>::iterator i = _items.find(key);
	if (i == _items.end())
		return;
	Debug::inventory() << "Removing " << item << " from inventory slot " << key << endl;
	_extrinsics_updated = false;
	if (i->second.count() > item.count()) {
		/* reduce stack */
		i->second.count(i->second.count() - item.count());
	} else {
		/* remove stack entirely */
		for (int a = 0; a < SLOTS; ++a) {
			if (_slots[a] == key) {
				_slots[a] = '\0';
				break;
			}
		}
		_items.erase(i);
	}
}

const unsigned long long int& Inventory::extrinsicsFromItems() {
	if (!_extrinsics_updated)
		updateExtrinsics();
	return _extrinsics_from_items;
}

const bool& Inventory::updated() {
	return _updated;
}

void Inventory::update() {
	_updated = false;
	_extrinsics_updated = false;
}

/* private methods */
void Inventory::setSlot(const unsigned char& key, const Item& item) {
	if (item.additional() == "being worn") {
		/* armor */
		map<const string, const data::Armor*>::const_iterator a = data::Armor::armors().find(item.name());
		if (a != data::Armor::armors().end()) {
			_slots[a->second->slot()] = key;
			return;
		}
		/* amulet */
		map<const string, const data::Amulet*>::const_iterator b = data::Amulet::amulets().find(item.name());
		if (b != data::Amulet::amulets().end()) {
			_slots[SLOT_AMULET] = key;
			return;
		}
	} else if (item.additional() == "wielded") {
		_slots[SLOT_WEAPON] = key;
	} else if (item.additional().find("weapon in ") == 0) {
		_slots[SLOT_WEAPON] = key;
	} else if (item.additional().find("wielded in other ") == 0) {
		_slots[SLOT_OFFHAND_WEAPON] = key;
	} else if (item.additional().find("on left ") == 0) {
		_slots[SLOT_LEFT_RING] = key;
	} else if (item.additional().find("on right ") == 0) {
		_slots[SLOT_RIGHT_RING] = key;
	}
}

void Inventory::updateExtrinsics() {
	/* figure out what extrinsics we get from items */
	for (int s = 0; s < SLOTS; ++s) {
		const Item& item = itemInSlot(s);
		if (item.count() <= 0)
			continue; // no item in slot
		map<const string, const data::Item*>::const_iterator i = data::Item::items().find(item.name());
		if (i == data::Item::items().end())
			continue; // item not in database, shouldn't happen very often
		_extrinsics_updated |= i->second->properties();
	}
	/* TODO: items that give extrinsics by only carrying them (ie. longbow of diana) */
	_extrinsics_updated = true;
}
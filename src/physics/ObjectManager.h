#pragma once

#include <vector>
#include <memory>

#include "Windy/Node.h"

namespace windy {
	class Level;
	class ObjectEntry;
}


namespace windy {
	class ObjectManager : public windy::Node{
	public:
		static std::shared_ptr<ObjectManager> create(std::shared_ptr<Level> level);

		virtual bool init();

		void resetEntryTable(std::vector<std::shared_ptr<ObjectEntry>> newEntries);

		virtual void update(float dt);

		void addEntry(std::shared_ptr<ObjectEntry> entry);

		std::vector<std::shared_ptr<ObjectEntry>> getObjectEntries() { return _objectEntries; }

	private:
		std::shared_ptr<Level> _level;
		std::vector<std::shared_ptr<ObjectEntry>> _objectEntries;

	};

}


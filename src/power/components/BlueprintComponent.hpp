#pragma once

#include <memory>

class Actor;
class NodeProcessor;

class BlueprintComponent {
public:
	BlueprintComponent(std::unique_ptr<NodeProcessor> nodeProcessor);
	
	NodeProcessor& node_processor() const;

	void update();

	void save_blueprint(const std::string& to_file);

	void load_blueprint(const std::string& from_file);

private:
	std::unique_ptr<NodeProcessor> mNodeProcessor;
};

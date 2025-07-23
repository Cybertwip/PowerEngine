#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <any>
#include <memory>
#include "peglib.h"

// A simple struct to hold information about each reflectable struct found.
struct ReflectableStruct {
	std::string name;
	std::vector<std::string> fields;
	std::vector<std::string> methods;
};

// A helper struct to pass member information (field or method) up the parse tree.
struct MemberInfo {
	enum Type { FIELD, METHOD, OTHER };
	Type type = OTHER;
	std::string name;
};


int main(int argc, char** argv) {
	if (argc != 4) {
		std::cerr << "Usage: generator <input_header_full_path> <output_cpp_full_path> <header_relative_path>\n";
		return 1;
	}
	const char* input_path = argv[1];
	const char* output_path = argv[2];
	
	std::string relative_header_path = argv[3];
	for (char& c : relative_header_path) {
		if (c == '\\') {
			c = '/';
		}
	}
	
	// --- Grammar to find structs and members marked for reflection ---
	// This grammar has been improved to be more efficient and robust.
	const auto grammar = R"(
		# Top-level rule: A file is a sequence of content, which can be a struct or other text.
		File             <- (Struct / OtherContent)*
	
		# A struct is defined by our attribute, with explicit whitespace/comment handling.
		Struct           <- ATTR_REFLECTABLE _ 'struct' _ Name _ '{' _ Members _ '};'
	
		# Members are a collection of fields, methods, or other things inside a struct.
		Members          <- (Field / Method / OtherInStruct)*
	
		# A field is marked with the 'field' attribute.
		Field            <- ATTR_FIELD _ Type _ Name _ Initializer? _ ';'
	
		# A method is marked with the 'method' attribute.
		Method           <- ATTR_METHOD _ (Type _)? Name _ '(' _ PList? _ ')' _ CQual? _ (FunctionBody / ';')
	
		# A function body can be either a block with braces or just a semicolon for declarations.
		FunctionBody     <- '{' Body '}'
		Body             <- ( [^{}] / FunctionBody )*
	
		# Basic language elements
		Name             <- < [a-zA-Z_][a-zA-Z0-9_:]* >
		Type             <- Name _ ('<' _ Type (_ ',' _ Type)* _ '>')? (_ ('&' / 'const' / '*'))*
		PList            <- Type _ Name (_ ',' _ PList)?
		CQual            <- 'const'
		Initializer      <- '=' (!';' .)*
	
		# Rules to consume content that we want to ignore.
		OtherContent     <- (!ATTR_REFLECTABLE .)+
		OtherInStruct    <- (!('}' / ATTR_FIELD / ATTR_METHOD) .)+
	
		# Attribute definitions with flexible spacing.
		ATTR_REFLECTABLE <- '[[' _ 'powergen' _ '::' _ 'reflectable' _ ']]'
		ATTR_FIELD       <- '[[' _ 'powergen' _ '::' _ 'field' _ ']]'
		ATTR_METHOD      <- '[[' _ 'powergen' _ '::' _ 'method' _ ']]'
	
		# Whitespace and comments are explicitly handled.
		_                <- ( [ \t\r\n] / CppComment )*
		CppComment       <- '//' [^\r\n]* / '/*' ( !'*/' . )* '*/'
	)";
	
	peg::parser parser;
	
	// Set a logger to see detailed parsing errors for the grammar itself.
	parser.set_logger([](size_t line, size_t col, const std::string& msg, const std::string &rule) {
		std::cerr << "Grammar error at " << line << ":" << col << " in rule '" << rule << "': " << msg << "\n";
	});
	
	if (!parser.load_grammar(grammar)) {
		std::cerr << "FATAL: Grammar is invalid, cannot proceed." << std::endl;
		return 1;
	}
	
	std::vector<ReflectableStruct> reflectables;
	
	// --- Setup Semantic Actions to build the data structure from the bottom up ---
	
	// The Field rule now returns a MemberInfo object.
	parser["Field"] = [](const peg::SemanticValues& sv) {
		return std::make_any<MemberInfo>(MemberInfo{MemberInfo::FIELD, sv.token_to_string(0)});
	};
	
	// The Method rule also returns a MemberInfo object.
	parser["Method"] = [](const peg::SemanticValues& sv) {
		return std::make_any<MemberInfo>(MemberInfo{MemberInfo::METHOD, sv.token_to_string(0)});
	};
	
	// The Struct rule is the final step. It receives the struct name as a token
	// and a flattened list of semantic values from its children (the MemberInfo objects).
	parser["Struct"] = [&](const peg::SemanticValues& sv) {
		ReflectableStruct current_struct;
		current_struct.name = sv.token_to_string(0);
		
		// Iterate through the semantic values. Each value returned from the Field or Method
		// rules will be an element in 'sv'.
		for (const auto& item_any : sv) {
			if (item_any.type() == typeid(MemberInfo)) {
				const auto& member = std::any_cast<MemberInfo>(item_any);
				if (member.type == MemberInfo::FIELD) {
					current_struct.fields.push_back(member.name);
				} else if (member.type == MemberInfo::METHOD) {
					current_struct.methods.push_back(member.name);
				}
			}
		}
		
		reflectables.push_back(current_struct);
		std::cout << "Successfully parsed struct: " << current_struct.name << "\n";
	};
	
	std::ifstream ifs(input_path);
	if (!ifs) {
		std::cerr << "Error: Could not open input file " << input_path << std::endl;
		return 1;
	}
	
	std::string content(std::istreambuf_iterator<char>(ifs), {});
	
	// Parse the file content. The semantic actions will populate the 'reflectables' vector.
	if (!parser.parse(content)) {
		std::cerr << "Error: Failed to parse file " << input_path << std::endl;
		return 1;
	}
	
	if (reflectables.empty()) {
		std::cout << "No reflectable structs found in " << input_path << ". No file will be generated." << std::endl;
		return 0;
	}
	
	// --- Generate refl-cpp and Power Reflection Registration Code ---
	std::filesystem::path out_p(output_path);
	if (out_p.has_parent_path()) {
		std::filesystem::create_directories(out_p.parent_path());
	}
	
	std::ofstream ofs(output_path);
	if (!ofs) {
		std::cerr << "Error: Could not open output file for writing: " << output_path << std::endl;
		return 1;
	}
	
	ofs << "// DO NOT EDIT - Code generated by 'generator' from " << relative_header_path << "\n\n";
	ofs << "#include \"" << relative_header_path << "\"\n";
	ofs << "#include \"PowerReflection.hpp\"\n\n";
	
	for (const auto& s : reflectables) {
		ofs << "// --- Reflection metadata for " << s.name << " ---\n";
		ofs << "REFL_AUTO(\n";
		ofs << "    type(" << s.name << ")";
		
		for (const auto& name : s.fields) {
			ofs << ",\n    field(" << name << ")";
		}
		for (const auto& name : s.methods) {
			ofs << ",\n    func(" << name << ")";
		}
		ofs << "\n)\n\n";
		
		ofs << "// Static registration helper for the Power Reflection system\n";
		ofs << "namespace {\n";
		ofs << "    struct AutoRegister_" << s.name << " {\n";
		ofs << "        AutoRegister_" << s.name << "() {\n";
		ofs << "            power::reflection::registerType<" << s.name << ">();\n";
		ofs << "        }\n";
		ofs << "    };\n";
		ofs << "    static AutoRegister_" << s.name << " auto_register_" << s.name << ";\n";
		ofs << "}\n\n";
		
		std::cout << "Generated Power Reflection code for " << s.name << " from " << input_path << std::endl;
	}
	
	return 0;
}

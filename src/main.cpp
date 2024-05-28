#include "app.h"

int Main(int argc, char** argv)
{
	std::filesystem::current_path(DEFAULT_CWD);

	App oac("OAC", argc, argv);
	
	if (oac.Create())
		return oac.Run();


//    App::get_instance()->init(1400, 800, "OAC");
//    App::get_instance()->loop();
//    App::destroy_instance();
    return 0;
}

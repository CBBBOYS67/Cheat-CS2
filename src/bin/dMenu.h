class DMenu
{
public:
	/// <summary>
	/// Handles drawing of actual dMenu elements onto the canvas. This function assumes the menu always shows up.
	/// </summary>
	static void draw();

	static ImVec2 relativeSize(float width, float height);
	// called once the game data is loaded and d3d hook installed.
	static void init(float a_screenWidth, float a_screenHeight);
	
	static inline bool initialized = false;

private:

	// Define an enum to represent the different tabs
	enum Tab
	{
		Trainer,
		AIM,
		Contact,
		Help
	};

	// Declare a variable to store the currently selected tab
	static inline Tab currentTab = Trainer;

	static inline ImVec2 mainWindowSize = { 0, 0 };

};


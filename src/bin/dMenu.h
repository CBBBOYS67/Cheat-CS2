class DMenu
{
public:
	static DMenu* getSingleton()
	{
		static DMenu singleton;
		return &singleton;
	}

	/// <summary>
	/// Handles drawing of actual dMenu elements onto the canvas. This function assumes the menu always shows up.
	/// </summary>
	void draw();

private:

	class Trainer
	{
	public:
		static float* getTimeOfDay();
		static void setTimeOfDay(float a_in);
	};
	// Define an enum to represent the different tabs
	enum Tab
	{
		Trainer,
		About,
		Contact,
		Help
	};

	// Declare a variable to store the currently selected tab
	Tab currentTab = Trainer;

	void menuTrainer();

	void menuConfig();

	void menuSetting();
	
};

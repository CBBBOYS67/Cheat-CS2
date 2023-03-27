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
};

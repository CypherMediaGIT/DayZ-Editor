class EditorWorldObject
{
	protected EntityAI m_WorldObject;
	EntityAI GetWorldObject() {
		return m_WorldObject;
	}
}


class EditorObject: EditorWorldObject
{
	protected EditorObjectData 				m_Data;
	protected ref EditorObjectMapMarker		m_EditorObjectMapMarker;
	protected ref EditorObjectWorldMarker	m_EditorObjectWorldMarker;
	protected ref EditorPlacedListItem 		m_EditorPlacedListItem;
	
	protected Object		m_BBoxLines[12];	
	protected Object 		m_BBoxBase;
	protected Object 		m_CenterLine;
	protected Object		m_BasePoint;
	
	protected ref array<ref EditorSnapPoint> m_SnapPoints = {};
	
	private vector m_LineCenters[12]; 
	private vector m_LineVerticies[8];
	
	static float line_width = 0.02;
	
	ref ScriptInvoker OnObjectSelected = new ScriptInvoker();
	ref ScriptInvoker OnObjectDeselected = new ScriptInvoker();

	void SetDisplayName(string display_name) {
		m_Data.DisplayName = display_name;
		EditorListItemController.Cast(m_EditorPlacedListItem.GetController()).Label = m_Data.DisplayName;
		m_EditorPlacedListItem.GetController().NotifyPropertyChanged("Label");
	}
	
	string GetDisplayName() { 
		return m_Data.DisplayName; 
	}
	
	string GetType() { 
		return m_Data.Type; 
	}
	
	int GetID() { 
		return m_Data.GetID(); 
	}

	Object GetWorldObject() 
	{
		if (!m_WorldObject) {
			EditorLog.Error("World Object was null! ID: %1", GetID().ToString());
		}
		
		return m_WorldObject;
	}
	
	void EditorObject(EditorObjectData data)
	{
		EditorLog.Trace("EditorObject " + data);
		m_Data = data;
		
		if (!m_Data.WorldObject) {
			m_WorldObject = GetGame().CreateObjectEx(m_Data.Type, m_Data.Position, ECE_LOCAL | ECE_CREATEPHYSICS);
			m_WorldObject.SetOrientation(m_Data.Orientation);
			m_WorldObject.SetFlags(EntityFlags.STATIC, true);
			m_Data.WorldObject = m_WorldObject;
		} else {
			m_WorldObject = m_Data.WorldObject;
		}
		
		if (GetEditor()) {
			GetEditor().GetSessionCache().Insert(m_Data.GetID(), m_Data);
		}
		
		
		vector clip_info[2];
		ClippingInfo(clip_info);
	
		m_LineVerticies[0] = clip_info[0];
		m_LineVerticies[1] = Vector(clip_info[0][0], clip_info[0][1], clip_info[1][2]);
		m_LineVerticies[2] = Vector(clip_info[1][0], clip_info[0][1], clip_info[1][2]);
		m_LineVerticies[3] = Vector(clip_info[1][0], clip_info[0][1], clip_info[0][2]);		
		m_LineVerticies[4] = Vector(clip_info[1][0], clip_info[1][1], clip_info[0][2]);
		m_LineVerticies[5] = clip_info[1];
		m_LineVerticies[6] = Vector(clip_info[0][0], clip_info[1][1], clip_info[1][2]);
		m_LineVerticies[7] = Vector(clip_info[0][0], clip_info[1][1], clip_info[0][2]);
				
		m_LineCenters[0] = AverageVectors(m_LineVerticies[0], m_LineVerticies[1]);
		m_LineCenters[1] = AverageVectors(m_LineVerticies[0], m_LineVerticies[3]);
		m_LineCenters[2] = AverageVectors(m_LineVerticies[0], m_LineVerticies[7]);
		m_LineCenters[3] = AverageVectors(m_LineVerticies[4], m_LineVerticies[7]);
		m_LineCenters[4] = AverageVectors(m_LineVerticies[6], m_LineVerticies[7]);
		
		m_LineCenters[5] = AverageVectors(m_LineVerticies[1], m_LineVerticies[2]);
		m_LineCenters[6] = AverageVectors(m_LineVerticies[1], m_LineVerticies[6]);
		m_LineCenters[7] = AverageVectors(m_LineVerticies[3], m_LineVerticies[2]);
		m_LineCenters[8] = AverageVectors(m_LineVerticies[3], m_LineVerticies[4]);
		
		m_LineCenters[9] = AverageVectors(m_LineVerticies[5], m_LineVerticies[2]);
		m_LineCenters[10] = AverageVectors(m_LineVerticies[5], m_LineVerticies[4]);		
		m_LineCenters[11] = AverageVectors(m_LineVerticies[5], m_LineVerticies[6]);
		
		vector base_point = AverageVectors(AverageVectors(m_LineVerticies[0], m_LineVerticies[1]), AverageVectors(m_LineVerticies[2], m_LineVerticies[3]));
		m_BasePoint = GetGame().CreateObjectEx("BoundingBoxBase", base_point, ECE_NONE);
		m_BasePoint.SetScale(0.001);
		
		AddChild(m_BasePoint, 0);
		
		for (int i = 0; i < 8; i++) {
			m_SnapPoints.Insert(new EditorSnapPoint(this, m_LineVerticies[i]));
		}

		if ((m_Data.Flags & EditorObjectFlags.BBOX) == EditorObjectFlags.BBOX) {
			//ShowBoundingBox();
		}
		
		// Map marker
		if ((m_Data.Flags & EditorObjectFlags.MAPMARKER) == EditorObjectFlags.MAPMARKER) {
			m_EditorObjectMapMarker = new EditorObjectMapMarker(this);
			GetEditor().GetEditorHud().GetTemplateController().InsertMapMarker(m_EditorObjectMapMarker);
		}	
		
		// World Object base marker
		if ((m_Data.Flags & EditorObjectFlags.OBJECTMARKER) == EditorObjectFlags.OBJECTMARKER) {
			m_EditorObjectWorldMarker = new EditorObjectWorldMarker(this);
		}
		
		// Browser item
		if ((m_Data.Flags & EditorObjectFlags.LISTITEM) == EditorObjectFlags.LISTITEM) {
			m_EditorPlacedListItem = new EditorPlacedListItem();
			m_EditorPlacedListItem.SetEditorObject(this);
			GetEditor().GetEditorHud().GetTemplateController().RightbarSpacerData.Insert(m_EditorPlacedListItem);
		}
	}
	
		
	void ~EditorObject()
	{
		EditorLog.Trace("~EditorObject");
		Update();
		
		HideBoundingBox();
		
		delete m_EditorObjectWorldMarker; 
		delete m_EditorPlacedListItem;
		delete m_EditorObjectMapMarker;
		
		delete OnObjectSelected;
		delete OnObjectDeselected;
			
		GetGame().ObjectDelete(m_WorldObject);
	}
			
	/*********
	* Events *
	*********/
	
	private bool m_IsSelected;
	bool IsSelected() return m_IsSelected;
	void OnSelected()
	{
		if (m_IsSelected) return;
		EditorLog.Trace("EditorObject::OnSelected");
		m_IsSelected = true;
		ShowBoundingBox();
		OnObjectSelected.Invoke(this);
	}
	
	void OnDeselected()
	{
		if (!m_IsSelected) return;
		EditorLog.Trace("EditorObject::OnDeselected");
		m_IsSelected = false;
		HideBoundingBox();
		OnObjectDeselected.Invoke(this);
	}
	
	EditorObjectData GetData() {
		return m_Data;
	}
	
	bool OnMouseEnter(int x, int y)	{
		return true;
	}
	
	bool OnMouseLeave(int x, int y) {
		return true;
	}

	vector GetPosition() { 
		return GetWorldObject().GetPosition(); 
	}
	
	void SetPosition(vector pos) 
	{ 
		GetWorldObject().SetPosition(pos);
		Update();
	}
	
	vector GetOrientation() { return GetWorldObject().GetOrientation(); }
	void SetOrientation(vector pos) 
	{ 
		GetWorldObject().SetOrientation(pos);
		Update();
	}
	
	void GetTransform(out vector mat[4]) 
	{ 
		GetWorldObject().GetTransform(mat); 
	}
	
	void SetTransform(vector mat[4]) 
	{ 		
		GetWorldObject().SetTransform(mat); 
		Update();
	}
	
	// temp bc GetScale doesnt work
	private float _scale = 1;
	void SetScale(float scale)
	{
		_scale = scale;
		vector mat[4];
		GetTransform(mat);
		mat[0][0] = scale;
		mat[1][1] = scale;
		mat[2][2] = scale;
		SetTransform(mat);
		
		//m_WorldObject.SetScale(scale);
		//m_WorldObject.Update();
	}
	
	float GetScale()
	{
		return _scale;
	}
	
	void Update() 
	{ 
		GetWorldObject().Update(); 
		
		if (m_Data) {
			m_Data.Position = GetPosition();
			m_Data.Orientation = GetOrientation();
		}
	}
	
	void PlaceOnSurfaceRotated(out vector trans[4], vector pos, float dx = 0, float dz = 0, float fAngle = 0, bool align = false) {
		
		EntityAI ent;
		if (Class.CastTo(ent, GetWorldObject())) {
			ent.PlaceOnSurfaceRotated(trans, pos, dx, dz, fAngle, align); 
		}
	}
	
	void ClippingInfo(out vector clip_info[2]) { 
		GetWorldObject().ClippingInfo(clip_info); 
	}
	
	void SetDirection(vector direction) { 
		GetWorldObject().SetDirection(direction); 
	}
	
	void AddChild(notnull IEntity child, int pivot, bool position_only = false) { 
		GetWorldObject().AddChild(child, pivot, position_only); 
	}
	
	vector GetTransformAxis(int axis) { 
		return GetWorldObject().GetTransformAxis(axis); 
	}
	
	string GetModelName() { 
		return GetWorldObject().GetModelName(); 
	}
	
	bool ListItemEnabled() { 
		return (m_Data.Flags & EditorObjectFlags.LISTITEM) == EditorObjectFlags.LISTITEM; 
	}
	
	bool ObjectMarkerEnabled() { 
		return (m_Data.Flags & EditorObjectFlags.OBJECTMARKER) == EditorObjectFlags.OBJECTMARKER; 
	}
	
	bool MapMarkerEnabled() { 
		return (m_Data.Flags & EditorObjectFlags.OBJECTMARKER) == EditorObjectFlags.OBJECTMARKER;
	}

	bool BoundingBoxEnabled() { 
		return (m_Data.Flags & EditorObjectFlags.BBOX) == EditorObjectFlags.BBOX; 
	}
	
	private bool m_Visible = true;
	bool IsVisible() {
		return m_Visible;
	}
	
	void Show(bool show) 
	{
		m_Visible = show;
		
		if (MapMarkerEnabled()) {
			m_EditorObjectMapMarker.Show(m_Visible);
		}
		
		if (ObjectMarkerEnabled()) {
			m_EditorObjectWorldMarker.Show(m_Visible);
		}
		
		if (m_Visible) {
			GetWorldObject().SetFlags(EntityFlags.VISIBLE | EntityFlags.SOLID | EntityFlags.TOUCHTRIGGERS, true);
		} else {
			GetWorldObject().ClearFlags(EntityFlags.VISIBLE | EntityFlags.SOLID | EntityFlags.TOUCHTRIGGERS, true);
		}
	}
	
	void ShowWorldObject(bool show) { 
		
		if (show) {
			GetWorldObject().SetFlags(EntityFlags.VISIBLE, false);
		} else {
			GetWorldObject().ClearFlags(EntityFlags.VISIBLE, false);
		}
	}
		
	vector GetBottomCenter()
	{		
		return m_BasePoint.GetWorldPosition();
	}
	
	float GetYDistance()
	{
		return ((GetPosition() - m_BasePoint.GetPosition())[1]);
	}
	
	vector GetTopCenter()
	{		
		vector clip_info[2];
		ClippingInfo(clip_info);
		vector result;
		vector up = GetTransformAxis(1);
		result = up * (vector.Distance(Vector(0, clip_info[0][1], 0), Vector(0, clip_info[1][1], 0)) / 2);
		result += GetPosition();
		return result;
	}
		

	Param3<int, vector, vector> GetTransformArray() 
	{
		return new Param3<int, vector, vector>(GetID(), GetPosition(), GetOrientation());
	}

	vector GetSize()
	{
		vector result;
		vector clip_info[2];
		ClippingInfo(clip_info);
		result[0] = Math.AbsFloat(clip_info[0][0]) + Math.AbsFloat(clip_info[1][0]);
		result[1] = Math.AbsFloat(clip_info[0][1]) + Math.AbsFloat(clip_info[1][1]);
		result[2] = Math.AbsFloat(clip_info[0][2]) + Math.AbsFloat(clip_info[1][2]);
		
		return result;
	}

	void SetTransformWithSnapping(vector transform[4])
	{	
		SetTransform(transform);
		

		// I cant wait to delete this... but not yet
		if (GetEditor().SnappingMode) {
			vector current_size = GetSize();
			vector current_pos = GetPosition();
			float snap_radius = 5;

			foreach (EditorObject editor_object: GetEditor().GetPlacedObjects()) {
				if (editor_object == this) continue;
				
				vector size = editor_object.GetSize();
				vector position = editor_object.GetPosition();
				
				if (vector.Distance(current_pos, position) < 100) {
					
					for (int i = 0; i < 12; i++) {
						vector pos = editor_object.m_BBoxLines[i].GetPosition();
						if (vector.Distance(pos, current_pos) < snap_radius) {
							SetPosition(pos);
							Update();
							return;
						}						
					}
				}
			}		
		}
		
		Update();
	}
		

	private bool BoundingBoxVisible;
	void ShowBoundingBox()
	{
		if (!BoundingBoxEnabled() || BoundingBoxVisible) return;
		EditorLog.Trace("EditorObject::ShowBoundingBox");
		BoundingBoxVisible = true;

		vector size = GetSize();
		vector clip_info[2];
		ClippingInfo(clip_info);
		vector position = AverageVectors(clip_info[0], clip_info[1]);
		
		for (int i = 0; i < 12; i++) {
			
			vector transform[4];			
			transform[3] = m_LineCenters[i];
			
			for (int j = 0; j < 3; j++) 
				transform[j][j] = ((position[j] == m_LineCenters[i][j])*size[j]/2) + line_width;						
			 
			m_BBoxLines[i] = EntityAI.Cast(GetGame().CreateObjectEx("BoundingBoxBase", m_LineCenters[i], ECE_LOCAL));
			m_BBoxLines[i].SetTransform(transform);			
			
			AddChild(m_BBoxLines[i], -1);
		}
		
		
		vector y_axis_mat[4];
		vector bottom_center = GetBottomCenter() - GetPosition();
		y_axis_mat[0][0] = line_width;
		y_axis_mat[1][1] = 1000;
		y_axis_mat[2][2] = line_width;
		y_axis_mat[3] = Vector(bottom_center[0], bottom_center[1] - y_axis_mat[1][1], bottom_center[2]);
		
		m_CenterLine = EntityAI.Cast(GetGame().CreateObjectEx("BoundingBoxBase", bottom_center, ECE_LOCAL));
		m_CenterLine.SetTransform(y_axis_mat);
		AddChild(m_CenterLine, -1);

		Update();
		
	}
	
	void HideBoundingBox()
	{
		if (!BoundingBoxEnabled() || !BoundingBoxVisible) return;
		EditorLog.Trace("EditorObject::HideBoundingBox");
		BoundingBoxVisible = false;
		
		for (int i = 0; i < 12; i++)
			GetGame().ObjectDelete(m_BBoxLines[i]);
		
		GetGame().ObjectDelete(m_BBoxBase);
		GetGame().ObjectDelete(m_CenterLine);	
	}
	

	bool SetAnimation(string anim_name)
	{
		EditorLog.Trace("EditorObject::SetAnimation");
		if (m_WorldObject.IsMan()) {
			DayZPlayerImplement.Cast(m_WorldObject).EditorAnimationStart(anim_name);
			return true;
		}
		
		return false;
	}

	
	void ResetAnimation()
	{
		EditorLog.Trace("EditorObject::SetAnimation");
		if (GetWorldObject().IsMan()) {
			DayZPlayerImplement.Cast(GetWorldObject()).EditorAnimationReset();
		}
	}
	
	void PauseSimulation(bool pause)
	{
		EditorLog.Trace("EditorObject::PauseSimulation");
		
		EntityAI ent;
		if (Class.CastTo(ent, GetWorldObject())) {
			ent.DisableSimulation(pause);
		}		
	}
	
	// Returns active Marker, either World or Map marker
	// Can return null
	EditorObjectMarker GetMarker()
	{
		//EditorLog.Trace("EditorObject::GetMarker");
		
		if (g_Editor.GetEditorHud().IsMapVisible()) {
			return m_EditorObjectMapMarker;
		}
		
		return m_EditorObjectWorldMarker;
	}
	
	EditorPlacedListItem GetListItem()
	{
		//EditorLog.Trace("EditorObject::GetListItem");
		return m_EditorPlacedListItem;
	}

	
}










typedef ref array<ref EditorBrush> EditorBrushSet;


class EditorBrush
{
	protected EntityAI m_BrushDecal;
	protected ref EditorBrushData m_BrushData;

	protected static float m_BrushRadius = 20;
	static void SetRadius(float radius) { m_BrushRadius = radius; }
	static float GetRadius() 
		return m_BrushRadius;
	
	protected static float m_BrushDensity = 0.5;
	static void SetDensity(float density) { m_BrushDensity = density; }
	static float GetDensity() 
		return m_BrushDensity;
	
	// Private members
	private vector m_LastMousePosition;

	
	private void EditorBrush(EditorBrushData settings = null)
	{
		EditorLog.Trace("EditorBrush");
		m_BrushData = settings;
		m_BrushDecal = EntityAI.Cast(GetGame().CreateObjectEx("BrushBase", vector.Zero, ECE_NONE));
		GetGame().GetUpdateQueue(CALL_CATEGORY_GUI).Insert(UpdateBrush);
	}

	
	void ~EditorBrush()
	{
		EditorLog.Trace("~EditorBrush");		
		GetGame().ObjectDelete(m_BrushDecal);
		GetGame().GetUpdateQueue(CALL_CATEGORY_GUI).Remove(UpdateBrush);
	}
	
	static EditorBrush Create(EditorBrushData settings) 
	{
		EditorLog.Trace("EditorBrush::Create " + settings.Name);

		if (settings.BrushClassName) {
			return EditorBrush.Cast(settings.BrushClassName.Spawn());
		}
	
		return new EditorBrush(settings);
	}	
	
	
	void SetBrushTexture(string texture)
	{
		EditorLog.Trace("EditorBrush::SetBrushTexture " + texture);
		m_BrushDecal.SetObjectTexture(0, texture);
		m_BrushDecal.Update();
	}
	
	
	void UpdateBrush()
	{
		if (GetEditor().IsPlacing()) return;
		
		set<Object> o;
		vector CurrentMousePosition = MousePosToRay(o, null, GetEditor().GetSettings().ObjectViewDistance, 0, true);
		
		Input input = GetGame().GetInput();

		vector transform[4] = {
			Vector(m_BrushRadius / 10, 0, 0),
			Vector(0, m_BrushRadius / 10, 0),
			Vector(0, 0, m_BrushRadius / 10),
			CurrentMousePosition
		};
		
		
		m_BrushDecal.SetTransform(transform);
		
		//if (GetEditor().GetUIManager().IsCursorOverUI()) return;
		/*
		if (input.LocalPress("UAFire")) {
			
		}*/
		
		if (GetWidgetUnderCursor()) 
			return;
		
		if (input.LocalPress("UAFire")) {
			OnMouseDown(CurrentMousePosition);
		}
		
		if (input.LocalValue("UAFire")) {
			DuringMouseDown(CurrentMousePosition);
		}
		
		if (input.LocalRelease("UAFire")) {
			OnMouseUp(CurrentMousePosition);
		}
	}
	
	
	
	void DuringMouseDown(vector position) 
	{ 
		if (vector.Distance(m_LastMousePosition, position) < (m_BrushRadius * Math.RandomFloat(0.5, 1))) return;
		m_LastMousePosition = position;
		
		EditorObjectDataMap data_set = new EditorObjectDataMap();
		
		for (int i = 0; i < m_BrushDensity * 100; i++) {
			
			
			vector pos = position;
			pos[0] = pos[0] + Math.RandomFloat(-m_BrushRadius / Math.PI, m_BrushRadius / Math.PI);
			pos[2] = pos[2] + Math.RandomFloat(-m_BrushRadius / Math.PI, m_BrushRadius / Math.PI);
			
			if (m_BrushData) {
				EditorBrushObject object_name = m_BrushData.GetRandomObject();
				vector transform[4];
				Math3D.MatrixIdentity4(transform);
				transform[3] = pos;
				EditorObjectData data = EditorObjectData.Create(object_name.Name, transform, EditorObjectFlags.NONE);
				data_set.Insert(data.GetID(), data);
			}
		}
		

		EditorObjectMap object_set = GetEditor().CreateObjects(data_set, true);
		i = 0;
		
		foreach (EditorObject editor_object: object_set) {
			
			
			pos = editor_object.GetPosition();			
			vector size = ObjectGetSize(editor_object.GetWorldObject());
			pos[1] = GetGame().SurfaceY(pos[0], pos[2]) + size[1] / 2.35; //+ m_BrushData.PlaceableObjectTypes[i].ZOffset;

			
			vector direction = Math3D.GetRandomDir();
			direction[1] = Math.RandomFloat(-0.05, 0.05);
			editor_object.SetDirection(direction);
			editor_object.SetPosition(pos);
			i++;
		}
	}
	
	void OnMouseUp(vector position)
	{
		// Reset mouse position when releasing mouse. 
		m_LastMousePosition = vector.Zero;
	}
	
	void OnMouseDown(vector position)
	{
		
	}
	
	string GetName() { 
		return m_BrushData.Name; 
	}
	
}



class DeleteBrush: EditorBrush
{	
	
	
	override void DuringMouseDown(vector position)
	{
		vector surface_normal = GetGame().SurfaceGetNormal(position[0], position[2]);
		vector contact_pos, contact_dir;
		int component;
		set<Object> results = new set<Object>();
		DayZPhysics.RaycastRV(position - surface_normal * 5, position + surface_normal * 500, contact_pos, contact_dir, component, results, null, null, false, false, 0, EditorBrush.GetRadius() / 2, CollisionFlags.ALLOBJECTS);
		GetEditor().ClearSelection();
		
		// todo this is deleting one at a time. use DeleteObjects smile :)
		foreach (Object r: results) {
					
			EditorObject eo = GetEditor().GetPlacedObjectById(r.GetID());
			if (eo != null) {
				GetEditor().DeleteObject(eo);
			} else {
				GetGame().ObjectDelete(r);
			}
		}		
	}

}

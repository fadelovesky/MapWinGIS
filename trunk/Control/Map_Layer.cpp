// Implementation of CMapView class (see other files as well)
// The properties and methods common for every layer are stored here.
// TODO: consider creation of Layers class as wrapper for this methods
// In this the members can copied to the new class. In the current class 
// simple redirections will be used. To make m_alllayers, m_activelayers available
// the variables can be initialized with the pointers to underlying data of Layers class
#pragma once
#include "stdafx.h"
#include "Map.h"
#include "Image.h"
#include "Grid.h"
#include "Shapefile.h"
#include "ImageLayerInfo.h"
#include "OgrLayer.h"
#include "OgrStyle.h"
#include "ShapefileHelper.h"
#include "OgrHelper.h"
#include "ComHelpers\ProjectionHelper.h"
#include "TableHelper.h"

// ************************************************************
//		GetNumLayers()
// ************************************************************
long CMapView::GetNumLayers()
{
	return _activeLayers.size();
}

// ************************************************************
//		LayerName()
// ************************************************************
BSTR CMapView::GetLayerName(LONG LayerHandle)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if( IsValidLayer(LayerHandle) )
	{	
		return W2BSTR( _allLayers[LayerHandle]->name );
	}
	else
	{	
		ErrorMessage(tkINVALID_LAYER_HANDLE);
		CString result;
		return result.AllocSysString();
	}
}
void CMapView::SetLayerName(LONG LayerHandle, LPCTSTR newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if( IsValidLayer(LayerHandle) )
	{	
		USES_CONVERSION;
		_allLayers[LayerHandle]->name = A2W(newVal);	// TODO: use Unicode
	}
	else
		ErrorMessage(tkINVALID_LAYER_HANDLE);
}

// ****************************************************
//		GetLayerDescription()
// ****************************************************
BSTR CMapView::GetLayerDescription(LONG LayerHandle)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if( IsValidLayer(LayerHandle) )
	{	
		return A2BSTR( _allLayers[LayerHandle]->description );
	}
	else
	{	
		ErrorMessage(tkINVALID_LAYER_HANDLE);
		CString result;
		return result.AllocSysString();
	}
}

// ****************************************************
//		SetLayerDescription()
// ****************************************************
void CMapView::SetLayerDescription(LONG LayerHandle, LPCTSTR newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (LayerHandle >= 0 && LayerHandle < (long)_allLayers.size())
	{
		Layer* layer = _allLayers[LayerHandle];
		layer->description = newVal;
	}
	else
	{
		this->ErrorMessage(tkINVALID_LAYER_HANDLE);
	}
}

// ************************************************************
//		LayerKey()
// ************************************************************
BSTR CMapView::GetLayerKey(long LayerHandle)
{
	USES_CONVERSION;
	if( IsValidLayer(LayerHandle) )
	{	
		return OLE2BSTR( _allLayers[LayerHandle]->key );
	}
	else
	{	
		ErrorMessage(tkINVALID_LAYER_HANDLE);
		CString result;
		return result.AllocSysString();
	}
}
void CMapView::SetLayerKey(long LayerHandle, LPCTSTR lpszNewValue)
{
	USES_CONVERSION;

	if( IsValidLayer(LayerHandle) )
	{	
		::SysFreeString(_allLayers[LayerHandle]->key);
		_allLayers[LayerHandle]->key = A2BSTR(lpszNewValue);
	}
	else
		ErrorMessage(tkINVALID_LAYER_HANDLE);
}

// ************************************************************
//		LayerPosition()
// ************************************************************
long CMapView::GetLayerPosition(long LayerHandle)
{
	if( IsValidLayer(LayerHandle) )
	{
		register int i;
		long endcondition = _activeLayers.size();
		for( i = 0; i < endcondition; i++ )
		{
			if( _activeLayers[i] == LayerHandle )
				return i;
		}

		return -1;
	}
	else
	{	
		ErrorMessage(tkINVALID_LAYER_HANDLE);
		return -1;
	}
}

// ************************************************************
//		LayerHandle()
// ************************************************************
long CMapView::GetLayerHandle(long LayerPosition)
{
	if( LayerPosition >= 0 && LayerPosition < (long)_activeLayers.size())
	{
		return _activeLayers[LayerPosition];
	}
	else
	{	
		ErrorMessage(tkINVALID_LAYER_POSITION);
		return -1;
	}
}

// ************************************************************
//		GetLayerVisible()
// ************************************************************
BOOL CMapView::GetLayerVisible(long LayerHandle)
{
	if( IsValidLayer(LayerHandle) )
	{	
		return ( _allLayers[LayerHandle]->flags & Visible );
	}
	else
	{	
		ErrorMessage(tkINVALID_LAYER_HANDLE);
		return FALSE;
	}
}

// ************************************************************
//		SetLayerVisible()
// ************************************************************
void CMapView::SetLayerVisible(long LayerHandle, BOOL bNewValue)
{
	if( IsValidLayer(LayerHandle) )
	{	
		if( bNewValue != FALSE )
			_allLayers[LayerHandle]->flags |= Visible;
		else
			_allLayers[LayerHandle]->flags = _allLayers[LayerHandle]->flags & ( 0xFFFFFFFF ^ Visible );

		// we need to refresh the buffer here
		if (_allLayers[LayerHandle]->IsImage())
		{
			if (_allLayers[LayerHandle]->object)
			{
				IImage * iimg = NULL;
				if (_allLayers[LayerHandle]->QueryImage(&iimg))
				{
					((CImageClass*)iimg)->_bufferReloadIsNeeded = true;
					iimg->Release();	
				}
			}
		}

		RedrawCore(RedrawAll, false);
	}
	else
	{	
		ErrorMessage(tkINVALID_LAYER_HANDLE);
	}
}

// ************************************************************
//		GetGetObject()
// ************************************************************
LPDISPATCH CMapView::GetGetObject(long LayerHandle)
{
	if( IsValidLayer(LayerHandle) )
	{	
		if (_allLayers[LayerHandle]->type == OgrLayerSource)
		{
			// for OGR layers we return underlying shapefile to make it compliant with existing client code
			IShapefile* sf = NULL;
			_allLayers[LayerHandle]->QueryShapefile(&sf);
			return (IDispatch*)sf;
		}

		IDispatch* obj = _allLayers[LayerHandle]->object;
		if (obj != NULL) obj->AddRef();
		return obj;
	}
	else
	{	
		ErrorMessage(tkINVALID_LAYER_HANDLE);
		return NULL;
	}
}

// ***************************************************************
//		AddLayerFromFilename()
// ***************************************************************
long CMapView::AddLayerFromFilename(LPCTSTR Filename, tkFileOpenStrategy openStrategy, VARIANT_BOOL visible)
{
	USES_CONVERSION;
	IDispatch* layer = NULL;
	CComBSTR bstr(Filename);
	_fileManager->Open(bstr, openStrategy, _globalCallback, &layer);
	if (layer) {
		long handle = AddLayer(layer, visible);
		layer->Release();
		return handle;
	}
	else {
		return -1;
	}
}

// ***************************************************************
//		AddLayerFromDatabase()
// ***************************************************************
long CMapView::AddLayerFromDatabase(LPCTSTR ConnectionString, LPCTSTR layerNameOrQuery, VARIANT_BOOL visible)
{
	USES_CONVERSION;
	IOgrLayer* layer = NULL;
	CComBSTR bstrConnection(ConnectionString);
	CComBSTR bstrQuery(layerNameOrQuery);
	_fileManager->OpenFromDatabase(bstrConnection, bstrQuery, &layer);
	if (layer) {
		long handle = AddLayer(layer, visible);
		layer->Release();
		return handle;
	}
	else {
		return -1;
	}
}

// ***************************************************************
//		AddLayerCore()
// ***************************************************************
int CMapView::AddLayerCore(Layer* layer)
{
	int layerHandle = -1;
	for (size_t i = 0; i < _allLayers.size(); i++)
	{
		if (!_allLayers[i])  // that means we can reuse it
		{
			layerHandle = i;
			_allLayers[i] = layer;
			break;
		}
	}

	if (layerHandle == -1)
	{
		layerHandle = _allLayers.size();
		_allLayers.push_back(layer);
	}

	_activeLayers.push_back(layerHandle);
	return layerHandle;
}

// ***************************************************************
//		AttachGlobalCallbackToLayers()
// ***************************************************************
void CMapView::AttachGlobalCallbackToLayers(IDispatch* object)
{
	if (!m_globalSettings.attachMapCallbackToLayers || !_globalCallback) return;

	CComPtr<IShapefile> ishp = NULL;
	CComPtr<IImage> iimg = NULL;
	CComPtr<IGrid> igrid = NULL;
	CComPtr<IOgrLayer> iogr = NULL;

	object->QueryInterface(IID_IShapefile, (void**)&ishp);
	object->QueryInterface(IID_IImage, (void**)&iimg);
	object->QueryInterface(IID_IGrid, (void**)&igrid);
	object->QueryInterface(IID_IOgrLayer, (void**)&iogr);

	CComPtr<ICallback> callback = NULL;
	
	if (igrid) {
		igrid->get_GlobalCallback(&callback);
		if (!callback) {
			igrid->put_GlobalCallback(_globalCallback);
		}
	}

	if (iimg) {
		iimg->put_GlobalCallback(_globalCallback);
		if (!callback) {
			iimg->put_GlobalCallback(_globalCallback);
		}
	}

	if (ishp) {
		ishp->put_GlobalCallback(_globalCallback);
		if (!callback) {
			ishp->put_GlobalCallback(_globalCallback);
		}
	}

	if (iogr)
	{
		iogr->put_GlobalCallback(_globalCallback);
		if (!callback) {
			iogr->put_GlobalCallback(_globalCallback);
		}
	}
}

// ***************************************************************
//		AddLayer()
// ***************************************************************
long CMapView::AddLayer(LPDISPATCH Object, BOOL pVisible)
{
	long layerHandle = -1;

	if( !Object )
	{	
		ErrorMessage(tkUNEXPECTED_NULL_PARAMETER);
		return layerHandle;
	}

	CComPtr<IOgrDatasource> ds = NULL;
	Object->QueryInterface(IID_IOgrDatasource, (void**)&ds);
	if (ds) 
	{
		bool mapIsEmpty = GetNumLayers() == 0;

		int layerCount = OgrHelper::GetLayerCount(ds);
		if (layerCount == 0) 
		{
			ErrorMessage(tkOGR_DATASOURCE_EMPTY);
			return layerHandle;
		}

		LockWindow(lmLock);

		for (int i = 0; i < layerCount; i++) 
		{
			CComPtr<IOgrLayer> layer = NULL;
			ds->GetLayer(i, VARIANT_FALSE, &layer);
			if (layer) {
				layerHandle = AddSingleLayer(layer, pVisible);		// recursion is here
			}
		}

		if (mapIsEmpty && GetNumLayers() > 0 && m_globalSettings.zoomToFirstLayer)
			ZoomToMaxExtents();

		LockWindow(lmUnlock);

		return layerHandle;   // returning the last layer handle
	}
	else {
		return AddSingleLayer(Object, pVisible);
	}
}

// ***************************************************************
//		AddSingleLayer()
// ***************************************************************
long CMapView::AddSingleLayer(LPDISPATCH Object, BOOL pVisible)
{
	long layerHandle = -1;

	IShapefile * ishp = NULL;
	Object->QueryInterface(IID_IShapefile, (void**)&ishp);

	IImage * iimg = NULL;
	Object->QueryInterface(IID_IImage, (void**)&iimg);

	IGrid * igrid = NULL;
	Object->QueryInterface(IID_IGrid, (void**)&igrid);

	IOgrLayer * iogr = NULL;
	Object->QueryInterface(IID_IOgrLayer, (void**)&iogr);

	if (!igrid && !iimg && !ishp && !iogr)
	{
		AfxMessageBox("Error: Interface Not Supported");
		ErrorMessage(tkINTERFACE_NOT_SUPPORTED);
		return -1;
	}

	AttachGlobalCallbackToLayers(Object);

	LockWindow(lmLock);

	Layer * l = NULL;

	if (ishp != NULL || iogr != NULL)
	{
		if (ishp)
		{
			tkShapefileSourceType type;
			ishp->get_SourceType(&type);
			if (type == sstUninitialized)
			{
				ErrorMessage(tkSHAPEFILE_UNINITIALIZED);
				LockWindow(lmUnlock);
				return -1;
			}

			CComPtr<ITable> table = NULL;
			ishp->get_Table(&table);
			if (table)
			{
				if (ShapefileHelper::GetNumShapes(ishp) != TableHelper::GetNumRows(table))
					ErrorMessage(tkDBF_RECORDS_SHAPES_MISMATCH);   // report it but allow to proceed
			}
		}

		l = new Layer();
		if (ishp) l->object = ishp;
		else l->object = iogr;
		l->type = ishp ? ShapefileLayer : OgrLayerSource;
		l->UpdateExtentsFromDatasource();
		l->flags = pVisible != FALSE ? l->flags | Visible : l->flags & (0xFFFFFFFF ^ Visible);

		layerHandle = AddLayerCore(l);
	}

	// grids aren't added directly; an image representation is created first 
	// using particular color scheme
	CStringW gridFilename = L"";

	if (igrid != NULL)
	{
		tkGridSourceType sourceType;
		igrid->get_SourceType(&sourceType);
		if (sourceType == tkGridSourceType::gstUninitialized)
		{
			ErrorMessage(tkGRID_NOT_INITIALIZED);
			LockWindow(lmUnlock);
			return -1;
		}

		CGrid* grid = (CGrid*)igrid;
		gridFilename = grid->GetFilename();
		CStringW proxyName = grid->GetProxyName();
		CStringW legendName = grid->GetProxyLegendName();
		CStringW imageName;

		PredefinedColorScheme coloring = m_globalSettings.GetGridColorScheme();

		CComPtr<IGridColorScheme> gridColorScheme = NULL;
		igrid->RetrieveOrGenerateColorScheme(gsrAuto, gsgGradient, coloring, &gridColorScheme);
		if (gridColorScheme)
		{
			// there is no proxy; either create a new one or opening directly
			ICallback* cback = NULL;
			igrid->get_GlobalCallback(&cback);
			tkGridProxyMode displayMode;
			igrid->get_PreferedDisplayMode(&displayMode);
			igrid->OpenAsImage(gridColorScheme, displayMode, cback, &iimg);
			if (cback) cback->Release();

			if (iimg)
			{
				// load transparency, etc
				// update color scheme from disk, as it's the one that is actually used; 
				// and not necessarily the one we passed to Grid.OpenAsImage
				VARIANT_BOOL vb;
				VARIANT_BOOL isProxy;
				iimg->get_IsGridProxy(&isProxy);
				CStringW legendName = isProxy ? grid->GetProxyLegendName() : grid->GetLegendName();

				CComPtr<IGridColorScheme> newScheme = NULL;
				ComHelper::CreateInstance(tkInterface::idGridColorScheme, (IDispatch**)&newScheme);

				CComBSTR bstrName(legendName);
				CComBSTR bstrElementName("GridColoringScheme");
				newScheme->ReadFromFile(bstrName, bstrElementName, &vb);
				if (vb)
				{
					((CImageClass*)iimg)->LoadImageAttributesFromGridColorScheme(newScheme);
				}
			}
		}
	}

	// it may be either directly opened image or the one created for the grid
	if (iimg != NULL)
	{
		tkImageSourceType type;
		iimg->get_SourceType(&type);
		if (type == istUninitialized)
		{
			ErrorMessage(tkIMAGE_UNINITIALIZED);
			LockWindow(lmUnlock);
			return -1;
		}

		l = new Layer();
		l->object = iimg;
		l->type = ImageLayer;
		l->flags = pVisible != FALSE ? l->flags | Visible : l->flags & (0xFFFFFFFF ^ Visible);
		l->UpdateExtentsFromDatasource();

		layerHandle = AddLayerCore(l);

		// try to save pixels in case image grouping is enabled
		if (_canUseImageGrouping)
		{
			if (!((CImageClass*)iimg)->SaveNotNullPixels())	// analyzing pixels...
				iimg->put_CanUseGrouping(VARIANT_FALSE);	//  don't try this image any more, before transparency values will be changed
		}
	}

	l->GrabLayerNameFromDatasource();

	GrabLayerProjection(l);

	bool diskSymbology = m_globalSettings.loadSymbologyOnAddLayer && l->IsDiskBased();
	CStringW symbologyName;
	if (diskSymbology)
	{
		// find out filename with symbology before file was substituted by reprojection
		CComBSTR bstrFilename;
		bstrFilename.Attach(GetLayerFilename(layerHandle));
		symbologyName = OLE2W(bstrFilename);
		symbologyName += L".mwsymb";
		symbologyName = Utility::FileExistsW(symbologyName) ? OLE2W(bstrFilename) : L"";
	}

	// do projection mismatch check
	if (!CheckLayerProjection(l, layerHandle))
	{
		RemoveLayerCore(layerHandle, false, false, true);
		LockWindow(lmUnlock);
		return -1;
	}

	if (m_globalSettings.loadSymbologyOnAddLayer)
	{
		// loading symbology
		if (diskSymbology)
		{
			if (symbologyName.GetLength() > 0) {
				CComBSTR desc;
				USES_CONVERSION;
				this->LoadLayerOptionsCore(W2A(symbologyName), layerHandle, "", &desc);
			}
		}
		else {
			LoadOgrStyle(l, layerHandle, L"", false);   // perhaps it's OGR database table
		}
	}

	if (l != NULL)
		FireLayerAdded(layerHandle);

	// set initial extents
	if (l != NULL && m_globalSettings.zoomToFirstLayer)
	{
		if (_activeLayers.size() == 1 && pVisible)
		{
			if (!l->IsEmpty())
				SetExtentsWithPadding(l->extents);
		}
	}

	ScheduleLayerRedraw();
	LockWindow(lmUnlock);
	return layerHandle;
}

// ***************************************************************
//		LoadOgrStyle()
// ***************************************************************
VARIANT_BOOL CMapView::LoadOgrStyle(Layer* layer, long layerHandle, CStringW name, bool reportError)
{
	VARIANT_BOOL result = VARIANT_FALSE;
	
	CComPtr<IOgrLayer> ogrLayer = NULL;
	layer->QueryOgrLayer(&ogrLayer);
	if (!ogrLayer)
		return result;

	if (OgrHelper::GetSourceType(ogrLayer) != ogrDbTable) 
		return result;

	CStringW xml = OgrHelper::Cast(ogrLayer)->LoadStyleXML(L"");
	if (xml.GetLength() == 0)
	{
		if (reportError)
			ErrorMessage(tkOGR_STYLE_NOT_FOUND);
		return result;
	}
	
	CPLXMLNode* root = CPLParseXMLString(Utility::ConvertToUtf8(xml));
	if (root) {
		CPLXMLNode* node = CPLGetXMLNode(root, "Layer");
		if (node) {
			result = DeserializeLayerOptionsCore(layerHandle, node);
		}
		CPLDestroyXMLNode(root);
	}
	return result;
}

// ***************************************************************
//		GrabLayerProjection()
// ***************************************************************
void CMapView::GrabLayerProjection( Layer* layer )
{
	// if we don't have a projection, let's try and grab projection from it
	if (_grabProjectionFromData && layer)
	{
		VARIANT_BOOL isEmpty = VARIANT_FALSE;
		GetMapProjection()->get_IsEmpty(&isEmpty);
		if (isEmpty)
		{
			IGeoProjection* gp =  layer->GetGeoProjection();
			if (gp)
			{
				gp->get_IsEmpty(&isEmpty);
				if (!isEmpty)
				{
					IGeoProjection* newProj = NULL;
					gp->Clone(&newProj);
					if (!newProj)
					{
						ErrorMsg(tkFAILED_TO_COPY_PROJECTION);
					}
					else
					{
						this->SetGeoProjection(newProj);
						newProj->Release();
					}
				}
				gp->Release();
			}
		}
	}
}

// ***************************************************************
//		CheckLayerProjection()
// ***************************************************************
bool CMapView::CheckLayerProjection( Layer* layer, int layerHandle )
{
	IGeoProjection* gpMap = GetMapProjection();

	CComPtr<IGeoProjection> gp = NULL;
	gp.Attach(layer->GetGeoProjection());
	
	if (ProjectionHelper::IsEmpty(gp))
	{
		tkMwBoolean cancel = m_globalSettings.allowLayersWithoutProjection ? blnFalse : blnTrue;
		FireLayerProjectionIsEmpty(layerHandle, &cancel);
		if (cancel == blnTrue) {
			ErrorMessage(tkMISSING_GEOPROJECTION);
			return false;
		}

		// even if user accepted the layer but it clearly doesn't fit we will reject it
		if (ProjectionHelper::IsGeographic(gpMap) && layer->extents.OutsideWorldBounds())
		{
			ErrorMessage(tkGEOGRAPHIC_PROJECTION_EXPECTED);
			return false;
		}
		return true;
	}
	
	// makes no sense to do the matching
	if (ProjectionHelper::IsEmpty(gpMap)) return true;     

	// mismatch testing
	CComPtr<IExtents> bounds = NULL;
	layer->GetExtentsAsNewInstance(&bounds);

	if (ProjectionHelper::IsSame(gpMap, gp, bounds, 20))
		return true;

	tkMwBoolean cancelAdding = m_globalSettings.allowProjectionMismatch ? blnFalse : blnTrue;
	tkMwBoolean reproject = m_globalSettings.reprojectLayersOnAdding ? blnTrue : blnFalse;
	FireProjectionMismatch(layerHandle, &cancelAdding, &reproject);

	if (cancelAdding) 
	{
		ErrorMessage(tkPROJECTION_MISMATCH);
		return false;
	}

	if (!reproject)	return true;	//basically ignore it
	
	if (layer->IsImage())
	{
		ErrorMessage(tkNO_REPROJECTION_FOR_IMAGES);
		return false;
	}
			
	if (layer->IsShapefile())
		return ReprojectLayer(layer, layerHandle);

	// for other layer types potentially added in the future
	return true;		
}

// ***************************************************************
//		ReprojectLayer()
// ***************************************************************
bool CMapView::ReprojectLayer(Layer* layer, int layerHandle)
{
	// let's try to do a transformation
	CComPtr<IShapefile> sf = NULL;
	if (!layer->QueryShapefile(&sf))
		return false;

	long numShapes = ShapefileHelper::GetNumShapes(sf);
	if (numShapes > m_globalSettings.maxReprojectionShapeCount) 
	{
		// OGR layers can potentially have millions of features, so let's be cautions not too start something to lengthy
		ErrorMessage(tkREPROJECTION_TOO_MUCH_SHAPES);		
		return false;
	}

	long count;
	IShapefile* sfNew = NULL;
	sf->Reproject(GetMapProjection(), &count, &sfNew);

	if (!sfNew || numShapes != count)
	{
		FireLayerReprojected(layerHandle, VARIANT_FALSE);
		if (sfNew) sfNew->Release();
		ErrorMessage(tkFAILED_TO_REPROJECT);
		return false;
	}
	
	// let's substitute original file with this one
	// don't close the original shapefile; use may still want to interact with it
	// release should be called twice, smart pointer won't allow it
	ShapefileHelper::Cast(sf)->Release();

	if (layer->type == OgrLayerSource)
	{
		CComPtr<IOgrLayer> ogr = NULL;
		layer->QueryOgrLayer(&ogr);
		if (ogr) {
			OgrHelper::Cast(ogr)->InjectShapefile(sfNew);
		}
	}
	else
	{
		layer->object = sfNew;
	}
	layer->UpdateExtentsFromDatasource();

	FireLayerReprojected(layerHandle, VARIANT_TRUE);

	return true;
}

// ***************************************************************
//		RemoveLayerCore()
// ***************************************************************
void CMapView::RemoveLayerCore(long LayerHandle, bool closeDatasources, bool fromRemoveAll, bool suppressEvent)
{
	try
	{
		if( !IsValidLayer(LayerHandle) ) 
		{
			ErrorMessage(tkINVALID_LAYER_HANDLE);
			return;
		}

		bool hadLayers = _activeLayers.size() > 0;
		if (LayerHandle >= (long)_allLayers.size()) return;
		Layer * l = _allLayers[LayerHandle];
		if (l == NULL) return;

		if (closeDatasources)
			l->CloseDatasources();
			
		for(unsigned int i = 0; i < _activeLayers.size(); i++ )
		{	
			if( _activeLayers[i] == LayerHandle )
			{	
				_activeLayers.erase( _activeLayers.begin() + i );
				break;
			}
		}

		try
		{
			// This may have been deleted already.
			if (_allLayers[LayerHandle] != NULL)
			{
				delete _allLayers[LayerHandle];
			}
		}
		catch(...)
		{
			Debug::WriteError("Exception during RemoveLayer");
		}

		_allLayers[LayerHandle] = NULL;

		if (_activeLayers.size() == 0 && hadLayers)
			ClearMapProjectionWithLastLayer();

		if (!suppressEvent)
			FireLayerRemoved(LayerHandle, fromRemoveAll ? VARIANT_TRUE : VARIANT_FALSE);

		Redraw();
	}
	catch(...)
	{
		Debug::WriteError("Exception during RemoveLayer");
	}
}

// ***************************************************************
//		RemoveLayer()
// ***************************************************************
void CMapView::RemoveLayer(long LayerHandle)
{
	RemoveLayerCore(LayerHandle, true);
}

// ***************************************************************
//		RemoveLayerWithoutClosing()
// ***************************************************************
void CMapView::RemoveLayerWithoutClosing(long LayerHandle)
{
	RemoveLayerCore(LayerHandle, false);
}

// ***************************************************************
//		RemoveAllLayers()
// ***************************************************************
void CMapView::RemoveAllLayers()
{
	LockWindow( lmLock );
	bool hadLayers = _activeLayers.size() > 0;
	
	for(unsigned int i = 0; i < _allLayers.size(); i++ )
	{
		if( IsValidLayer(i) )
		{
			RemoveLayerCore(i, true, true);
		}
	}
	_allLayers.clear();

	LockWindow( lmUnlock );

	_activeLayerPosition = 0;

	Redraw();
}

// ***************************************************************
//		MoveLayerUp()
// ***************************************************************
BOOL CMapView::MoveLayerUp(long InitialPosition)
{
	if( InitialPosition >= 0 && InitialPosition < (long)_activeLayers.size() )
	{	
		long layerHandle = _activeLayers[InitialPosition];

		_activeLayers.erase( _activeLayers.begin() + InitialPosition );

		long newPos = InitialPosition + 1;
		if( newPos > (long)_activeLayers.size() )
			newPos = _activeLayers.size();

		_activeLayers.insert( _activeLayers.begin() + newPos, layerHandle );

		Redraw();
		return TRUE;
	}
	else
	{	
		ErrorMessage(tkINVALID_LAYER_POSITION);
		return FALSE;
	}
}

// ***************************************************************
//		MoveLayerDown()
// ***************************************************************
BOOL CMapView::MoveLayerDown(long InitialPosition)
{
	if( InitialPosition >= 0 && InitialPosition < (long)_activeLayers.size() )
	{	
		long layerHandle = _activeLayers[InitialPosition];
		_activeLayers.erase( _activeLayers.begin() + InitialPosition );

		long newPos = InitialPosition - 1;
		if( newPos < 0 )
			newPos = 0;

		_activeLayers.insert( _activeLayers.begin() + newPos, layerHandle );

		Redraw();

		return TRUE;
	}
	else
	{	
		ErrorMessage(tkINVALID_LAYER_POSITION);
		return FALSE;
	}
}

// ***************************************************************
//		MoveLayer()
// ***************************************************************
BOOL CMapView::MoveLayer(long InitialPosition, long TargetPosition)
{
	if(InitialPosition == TargetPosition)
		return TRUE;

	if( InitialPosition >= 0 && InitialPosition < (long)_activeLayers.size() &&  
		TargetPosition >= 0 && TargetPosition < (long)_activeLayers.size())
	{
		long layerHandle = _activeLayers[InitialPosition];

		_activeLayers.erase( _activeLayers.begin() + InitialPosition );
		_activeLayers.insert( _activeLayers.begin() + TargetPosition, layerHandle );

		Redraw();

		return TRUE;
	}
	else
	{	
		ErrorMessage(tkINVALID_LAYER_POSITION);
		return FALSE;
	}
}

// ***************************************************************
//		MoveLayerTop()
// ***************************************************************
BOOL CMapView::MoveLayerTop(long InitialPosition)
{
	if( InitialPosition >= 0 && InitialPosition < (long)_activeLayers.size() )
	{	
		long layerHandle = _activeLayers[InitialPosition];
		_activeLayers.erase( _activeLayers.begin() + InitialPosition );
		_activeLayers.push_back(layerHandle);

		Redraw();

		return TRUE;
	}
	else
	{	
		ErrorMessage(tkINVALID_LAYER_POSITION);
		return FALSE;
	}
}

// ***************************************************************
//		MoveLayerBottom()
// ***************************************************************
BOOL CMapView::MoveLayerBottom(long InitialPosition)
{
	if( InitialPosition >= 0 && InitialPosition < (long)_activeLayers.size() )
	{	
		long layerHandle = _activeLayers[InitialPosition];
		_activeLayers.erase( _activeLayers.begin() + InitialPosition );
		_activeLayers.push_front(layerHandle);

		Redraw();

		return TRUE;
	}
	else
	{	
		ErrorMessage(tkINVALID_LAYER_POSITION);
		return FALSE;
	}
}

// ***************************************************************
//		ReSourceLayer()
// ***************************************************************
void CMapView::ReSourceLayer(long LayerHandle, LPCTSTR newSrcPath)
{
	USES_CONVERSION;

	if (IsValidLayer(LayerHandle))
	{	
		Layer * l = _allLayers[LayerHandle];
		CString newFile = newSrcPath;
		CComBSTR bstrName(newFile);
		VARIANT_BOOL rt;

		if (l->IsShapefile())
		{
			IShapefile * sf = NULL;
			if (!l->QueryShapefile(&sf)) return;
			sf->Resource(bstrName, &rt);

			IExtents * box = NULL;
			sf->get_Extents(&box);
			double xm,ym,zm,xM,yM,zM;
			box->GetBounds(&xm,&ym,&zm,&xM,&yM,&zM);
			l->extents = Extent(xm,xM,ym,yM);
			box->Release();
			box = NULL;
			sf->Release();
		}
		else if(l->IsImage())
		{
			IImage * iimg = NULL;
			
			if (!l->QueryImage(&iimg)) return;
			
			iimg->Resource(bstrName, &rt);
			iimg->Release();
		}
		else
			return;

		Redraw();
	}
	else
	{	
		ErrorMessage(tkINVALID_LAYER_HANDLE);
	}
}

// ****************************************************************** 
//		LayerMaxVisibleScale
// ****************************************************************** 
DOUBLE CMapView::GetLayerMaxVisibleScale(LONG LayerHandle)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (LayerHandle >= 0 && LayerHandle < (long)_allLayers.size())
	{
		Layer* layer = _allLayers[LayerHandle];
		return layer->maxVisibleScale;
	}
	else
	{
		this->ErrorMessage(tkINVALID_LAYER_HANDLE);
		return 0.0;
	}
}

void CMapView::SetLayerMaxVisibleScale(LONG LayerHandle, DOUBLE newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (LayerHandle >= 0 && LayerHandle < (long)_allLayers.size())
	{
		Layer* layer = _allLayers[LayerHandle];
		layer->maxVisibleScale = newVal;
	}
	else
	{
		this->ErrorMessage(tkINVALID_LAYER_HANDLE);
	}	
}

// ****************************************************************** 
//		LayerMinVisibleScale
// ****************************************************************** 
DOUBLE CMapView::GetLayerMinVisibleScale(LONG LayerHandle)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (LayerHandle >= 0 && LayerHandle < (long)_allLayers.size())
	{
		Layer* layer = _allLayers[LayerHandle];
		return layer->minVisibleScale;
	}
	else
	{
		this->ErrorMessage(tkINVALID_LAYER_HANDLE);
		return 0.0;
	}
}

void CMapView::SetLayerMinVisibleScale(LONG LayerHandle, DOUBLE newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (LayerHandle >= 0 && LayerHandle < (long)_allLayers.size())
	{
		Layer* layer = _allLayers[LayerHandle];
		layer->minVisibleScale = newVal;
	}
	else
	{
		this->ErrorMessage(tkINVALID_LAYER_HANDLE);
	}
}

// ****************************************************************** 
//		LayerMinVisibleZoom
// ****************************************************************** 
int CMapView::GetLayerMinVisibleZoom(LONG LayerHandle)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (LayerHandle >= 0 && LayerHandle < (long)_allLayers.size())
	{
		Layer* layer = _allLayers[LayerHandle];
		return layer->minVisibleZoom;
	}
	else
	{
		this->ErrorMessage(tkINVALID_LAYER_HANDLE);
		return -1;
	}
}

void CMapView::SetLayerMinVisibleZoom(LONG LayerHandle, int newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (LayerHandle >= 0 && LayerHandle < (long)_allLayers.size())
	{
		Layer* layer = _allLayers[LayerHandle];
		if (newVal < 0) newVal = 0;
		if (newVal > 18) newVal = 18;
		layer->minVisibleZoom = newVal;
	}
	else
	{
		this->ErrorMessage(tkINVALID_LAYER_HANDLE);
	}
}

// ****************************************************************** 
//		LayerMaxVisibleScale
// ****************************************************************** 
int CMapView::GetLayerMaxVisibleZoom(LONG LayerHandle)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (LayerHandle >= 0 && LayerHandle < (long)_allLayers.size())
	{
		Layer* layer = _allLayers[LayerHandle];
		return layer->maxVisibleZoom;
	}
	else
	{
		this->ErrorMessage(tkINVALID_LAYER_HANDLE);
		return -1;
	}
}

void CMapView::SetLayerMaxVisibleZoom(LONG LayerHandle, int newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (LayerHandle >= 0 && LayerHandle < (long)_allLayers.size())
	{
		Layer* layer = _allLayers[LayerHandle];
		if (newVal < 0) newVal = 0;
		if (newVal > 100) newVal = 100;
		layer->maxVisibleZoom = newVal;
	}
	else
	{
		this->ErrorMessage(tkINVALID_LAYER_HANDLE);
	}	
}

// ****************************************************************** 
//		LayerDynamicVisibility
// ****************************************************************** 
VARIANT_BOOL CMapView::GetLayerDynamicVisibility(LONG LayerHandle)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (LayerHandle >= 0 && LayerHandle < (long)_allLayers.size())
	{
		Layer* layer = _allLayers[LayerHandle];
		return layer->dynamicVisibility;
	}
	else
	{
		this->ErrorMessage(tkINVALID_LAYER_HANDLE);
		return VARIANT_FALSE;
	}
}

void CMapView::SetLayerDynamicVisibility(LONG LayerHandle, VARIANT_BOOL newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (LayerHandle >= 0 && LayerHandle < (long)_allLayers.size())
	{
		Layer* layer = _allLayers[LayerHandle];
		layer->dynamicVisibility = newVal?true:false;
	}
	else
	{
		this->ErrorMessage(tkINVALID_LAYER_HANDLE);
	}
}

#pragma region Serialization
// ********************************************************
//		DeserializeLayerCore()
// ********************************************************
// Loads layer based on the filename; return layer handle
int CMapView::DeserializeLayerCore(CPLXMLNode* node, CStringW ProjectName, bool utf8Filenames, IStopExecution* callback)
{
	const char* nameA = CPLGetXMLValue( node, "Filename", NULL );
	
	CStringW filename = Utility::XmlFilenameToUnicode(nameA, utf8Filenames);

	wchar_t buffer[4096] = L""; 
    DWORD retval = GetFullPathNameW(filename, 4096, buffer, NULL);
	if (retval > 4096)
		return -1;
	
	filename = buffer;
	
	bool visible = false;
	CString s = CPLGetXMLValue( node, "LayerVisible", NULL );
	if (s != "") visible = atoi(s) == 0 ? false : true;

	long layerHandle = -1;
	VARIANT_BOOL vb = VARIANT_FALSE;

	LayerType layerType = UndefinedLayer;
	s = CPLGetXMLValue( node, "LayerType", NULL );
	if (_stricmp(s, "Shapefile") == 0)
	{
		layerType = ShapefileLayer;
	}
	else if (_stricmp(s, "Image") == 0)
	{
		layerType = ImageLayer;
	}
	else if (_stricmp(s, "OgrLayer") == 0)
	{
		layerType = OgrLayerSource;
	}
	
	if (layerType == ShapefileLayer)
	{
		// opening shapefile
		CComPtr<IShapefile> sf = NULL;
		ComHelper::CreateInstance(idShapefile, (IDispatch**)&sf);
		
		if (sf) 
		{
			CComBSTR bstrFilename;
			if (filename.GetLength() == 0)
			{
				// shapefile type is arbitrary; the correct one will be supplied on deserialization
				bstrFilename = L"";
				sf->CreateNew(bstrFilename, ShpfileType::SHP_POLYGON, &vb);
			}
			else
			{
				bstrFilename = filename;
				sf->Open(bstrFilename, NULL, &vb);
			}
		
			if (vb)
			{
				layerHandle = this->AddLayer(sf, (BOOL)visible);
				
				CPLXMLNode* nodeShapefile = CPLGetXMLNode(node, "ShapefileClass");
				if (nodeShapefile)
				{
					IShapefile* isf = sf;
					((CShapefile*)isf)->DeserializeCore(VARIANT_TRUE, nodeShapefile);
				}
			}
		}
	}
	else if (layerType == ImageLayer)
	{
		// opening image
		IImage* img = NULL;
		CoCreateInstance( CLSID_Image, NULL, CLSCTX_INPROC_SERVER, IID_IImage, (void**)&img );
		
		if (img) 
		{
			CComBSTR name(filename);
			img->Open(name, USE_FILE_EXTENSION, VARIANT_FALSE, NULL, &vb);
		}
		
		if (vb)
		{
			layerHandle = this->AddLayer(img, (BOOL)visible);
			img->Release();
			CPLXMLNode* nodeImage = CPLGetXMLNode(node, "ImageClass");
			if (nodeImage)
			{
				((CImageClass*)img)->DeserializeCore(nodeImage);
			}
		}
	}
	else if (layerType == OgrLayerSource)
	{
		IOgrLayer* layer = NULL;
		ComHelper::CreateInstance(idOgrLayer, (IDispatch**)&layer);
		if (layer) 
		{
			CPLXMLNode* nodeOgrLayer = CPLGetXMLNode(node, "OgrLayerClass");
			if (nodeOgrLayer)
			{
				((COgrLayer*)layer)->DeserializeCore(nodeOgrLayer);
				layerHandle = AddLayer(layer, (BOOL)visible);
			}
			layer->Release();
		}
	}
	else
	{
		// unsupported layer type
		return -1;
	}

	if(layerHandle != -1) 
	{
		s = CPLGetXMLValue( node, "LayerName", NULL );
		_allLayers[layerHandle]->name = Utility::ConvertFromUtf8(s);

		s = CPLGetXMLValue( node, "DynamicVisibility", NULL );
		_allLayers[layerHandle]->dynamicVisibility = (s != "") ? (atoi(s) == 0 ? false : true) : false;
		
		s = CPLGetXMLValue( node, "MaxVisibleScale", NULL );
		_allLayers[layerHandle]->maxVisibleScale = (s != "") ? Utility::atof_custom (s) : MAX_LAYER_VISIBLE_SCALE;

		s = CPLGetXMLValue( node, "MinVisibleScale", NULL );
		_allLayers[layerHandle]->minVisibleScale = (s != "") ? Utility::atof_custom (s) : 0.0;

		s = CPLGetXMLValue( node, "MaxVisibleZoom", NULL );
		_allLayers[layerHandle]->maxVisibleZoom = (s != "") ? atoi(s) : 18;

		s = CPLGetXMLValue( node, "MinVisibleZoom", NULL );
		_allLayers[layerHandle]->minVisibleZoom = (s != "") ? atoi(s) : 0;

		s = CPLGetXMLValue( node, "LayerKey", NULL );
		this->SetLayerKey(layerHandle, s);

		s = CPLGetXMLValue( node, "LayerDescription", NULL );
		this->SetLayerDescription(layerHandle, s);
	}

	return layerHandle;
}

// ********************************************************
//		SerializeLayer()
// ********************************************************
// Filename isn't saved
BSTR CMapView::SerializeLayerOptions(LONG LayerHandle)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	USES_CONVERSION;

	CString str = "";
	CPLXMLNode* nodeLayer = SerializeLayerCore(LayerHandle, "");
	if (nodeLayer)
	{
		char* s = CPLSerializeXMLTree(nodeLayer);
		if (s)
		{
			str.Append(s);
			CPLFree(s);
		}
		CPLDestroyXMLNode(nodeLayer);
	}
	return A2BSTR(str);
}

// ********************************************************
//		SerializeLayerCore()
// ********************************************************
// For map state generation
CPLXMLNode* CMapView::SerializeLayerCore(LONG LayerHandle, CStringW Filename)
{
	USES_CONVERSION;
	
	if (LayerHandle < 0 || LayerHandle >= (long)_allLayers.size())
	{
		this->ErrorMessage(tkINVALID_LAYER_HANDLE);
		return NULL;
	}
		
	CPLXMLNode* psLayer = CPLCreateXMLNode(NULL, CXT_Element, "Layer");
	if (psLayer)
	{
		CString s;
		Layer* layer = _allLayers[LayerHandle];
		if (layer)
		{
			switch (layer->type)
			{
				case OgrLayerSource:
					s = "OgrLayer";
					break;
				case ImageLayer: 
					s = "Image";
					break;
				case ShapefileLayer:
					s = "Shapefile";
					break;
				case UndefinedLayer:
					s = "Undefined";
					break;
			}
			Utility::CPLCreateXMLAttributeAndValue( psLayer, "LayerType", s);
			Utility::CPLCreateXMLAttributeAndValue( psLayer, "LayerName", layer->name);
			
			Utility::CPLCreateXMLAttributeAndValue( psLayer, "LayerVisible", CPLString().Printf("%d", (int)(layer->flags & Visible) ));
			
			if (OLE2A(layer->key) != "")
				Utility::CPLCreateXMLAttributeAndValue( psLayer, "LayerKey", OLE2CA(layer->key));

			if (layer->description != "")
				Utility::CPLCreateXMLAttributeAndValue( psLayer, "LayerDescription", layer->description);

			if (layer->dynamicVisibility != false)
				Utility::CPLCreateXMLAttributeAndValue( psLayer, "DynamicVisibility", CPLString().Printf("%d", (int)layer->dynamicVisibility));

			if (layer->minVisibleScale != 0.0)
				Utility::CPLCreateXMLAttributeAndValue( psLayer, "MinVisibleScale", CPLString().Printf("%f", layer->minVisibleScale));
			
			if (layer->maxVisibleScale != 100000000.0)
				Utility::CPLCreateXMLAttributeAndValue( psLayer, "MaxVisibleScale", CPLString().Printf("%f", layer->maxVisibleScale));
			
			if (layer->minVisibleZoom != 0)
				Utility::CPLCreateXMLAttributeAndValue( psLayer, "MinVisibleZoom", CPLString().Printf("%d", layer->minVisibleZoom));

			if (layer->maxVisibleZoom != 18)
				Utility::CPLCreateXMLAttributeAndValue( psLayer, "MaxVisibleZoom", CPLString().Printf("%d", layer->maxVisibleZoom));

			IOgrLayer* ogr = NULL;
			layer->QueryOgrLayer(&ogr);
			if (ogr)
			{
				CPLXMLNode* node = ((COgrLayer*)ogr)->SerializeCore("OgrLayerClass");
				ogr->Release();
				if (node != NULL)
				{
					CPLAddXMLChild(psLayer, node);
				}
			}
			else
			{
				// retrieving filename
				IImage* img = NULL;
				CComPtr<IShapefile> sf = NULL;
				layer->QueryShapefile(&sf);
				layer->QueryImage(&img);

				if (sf || img)
				{
					CPLXMLNode* node = NULL;

					if (sf)
					{
						IShapefile* isf = sf;
						node = ((CShapefile*)isf)->SerializeCore(VARIANT_TRUE, "ShapefileClass", true);
					}
					else
					{
						CComBSTR bstr;
						img->get_SourceFilename(&bstr);
						CStringW sourceName = Utility::GetNameFromPath(OLE2W(bstr));
						Utility::CPLCreateXMLAttributeAndValue(psLayer, "SourceName", sourceName);

						node = ((CImageClass*)img)->SerializeCore(VARIANT_FALSE, "ImageClass");

						img->Release();
					}

					Utility::CPLCreateXMLAttributeAndValue(psLayer, "Filename", Filename);
					if (node)
					{
						CPLAddXMLChild(psLayer, node);
					}
				}
			}
		}
	}
	return psLayer;
}

// ********************************************************
//		DeserializeLayerOptions()
// ********************************************************
// Restores options, but doesn't add layer
VARIANT_BOOL CMapView::DeserializeLayerOptions(LONG LayerHandle, LPCTSTR newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	USES_CONVERSION;

	CString s = newVal;
	VARIANT_BOOL retval = VARIANT_FALSE;
	CPLXMLNode* node = CPLParseXMLString(s.GetString());
	if (node)
	{
		retval = DeserializeLayerOptionsCore(LayerHandle, node);
		CPLDestroyXMLNode( node );
	}
	return retval;
}

// ********************************************************
//		DeserializeLayerOptionsCore()
// ********************************************************
VARIANT_BOOL CMapView::DeserializeLayerOptionsCore(LONG LayerHandle, CPLXMLNode* node)
{
	if (LayerHandle < 0 || LayerHandle >= (long)_allLayers.size())
	{
		this->ErrorMessage(tkINVALID_LAYER_HANDLE);
		return VARIANT_FALSE;
	}
	
	if (!node)
	{
		ErrorMessage(tkINVALID_FILE);
		return VARIANT_FALSE;
	}
	
	node = CPLGetXMLNode(node, "=Layer");
	if (!node)
	{
		ErrorMessage(tkINVALID_FILE);
		return VARIANT_FALSE;
	}

	// layer type in the file
	LayerType layerType = UndefinedLayer;
	CString s = CPLGetXMLValue( node, "LayerType", NULL );
	
	if (_stricmp(s.GetString(), "Shapefile") == 0) 
		layerType = ShapefileLayer;
	else if (_stricmp(s.GetString(), "Image") == 0)
		layerType = ImageLayer;
	else if (_stricmp(s.GetString(), "OgrLayer") == 0)
		layerType = OgrLayerSource;

	if (layerType == UndefinedLayer)
	{
		ErrorMessage(tkINVALID_FILE);
		return VARIANT_FALSE;
	}

	// actual layer type
	bool injectShapefileToOgr = false;	
	Layer* layer = _allLayers[LayerHandle];
	if (layer->IsOgrLayer() && layerType == ShapefileLayer)
	{
		injectShapefileToOgr = true;
	}
	else if (layer->type != layerType)
	{
		ErrorMessage(tkINVALID_FILE);
		return VARIANT_FALSE;
	}

	// layer options
	s = CPLGetXMLValue( node, "LayerVisible", NULL );
	if (s != "") 
	{
		BOOL val = atoi(s);
		if( val )
			_allLayers[LayerHandle]->flags |= Visible;
		else
			_allLayers[LayerHandle]->flags = _allLayers[LayerHandle]->flags & ( 0xFFFFFFFF ^ Visible );
	}

	s = CPLGetXMLValue( node, "DynamicVisibility", NULL );
	_allLayers[LayerHandle]->dynamicVisibility = (s != "") ? (atoi(s) == 0 ? false : true) : false;
	
	s = CPLGetXMLValue( node, "MaxVisibleScale", NULL );
	_allLayers[LayerHandle]->maxVisibleScale = (s != "") ? Utility::atof_custom(s) : 100000000.0;	// todo use constant

	s = CPLGetXMLValue( node, "MinVisibleScale", NULL );
	_allLayers[LayerHandle]->minVisibleScale = (s != "") ? Utility::atof_custom(s) : 0.0;

	s = CPLGetXMLValue(node, "MinVisibleZoom", NULL);
	_allLayers[LayerHandle]->minVisibleZoom = (s != "") ? atoi(s) : 0;

	s = CPLGetXMLValue(node, "MaxVisibleZoom", NULL);
	_allLayers[LayerHandle]->maxVisibleZoom = (s != "") ? atoi(s) : 18;


	s = CPLGetXMLValue( node, "LayerKey", NULL );
	this->SetLayerKey(LayerHandle, s);

	s = CPLGetXMLValue( node, "LayerDescription", NULL );
	this->SetLayerDescription(LayerHandle, s);

	bool retVal = false;
	if (layerType == OgrLayerSource)
	{
		IOgrLayer* ogr = NULL;
		if (layer->QueryOgrLayer(&ogr))
		{
			node = CPLGetXMLNode(node, "OgrLayerClass");
			if (node)
			{
				retVal = ((COgrLayer*)ogr)->DeserializeOptions(node);
			}
			ogr->Release();
		}
	}
	else if (layerType == ShapefileLayer)
	{
		CComPtr<IShapefile> sf = NULL;
		if (layer->QueryShapefile(&sf))
		{
			node = CPLGetXMLNode(node, "ShapefileClass");
			if (node)
			{
				IShapefile* isf = sf;
				retVal = ((CShapefile*)isf)->DeserializeCore(VARIANT_TRUE, node);
			}
		}
	}
	else if (layerType == ImageLayer )
	{
		IImage* img = NULL;
		if (layer->QueryImage(&img))
		{
			node = CPLGetXMLNode(node, "ImageClass");
			if (node)
			{
				retVal =((CImageClass*)img)->DeserializeCore(node);
			}
			img->Release();
		}
	}
	else
	{
		return VARIANT_FALSE;
	}

	FireMapState(LayerHandle);

	return retVal ? VARIANT_TRUE : VARIANT_FALSE;
}

// *********************************************************
//		GetLayerFilename()
// *********************************************************
BSTR CMapView::GetLayerFilename(LONG layerHandle)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	BSTR filename;

	if (layerHandle < 0 || layerHandle >= (long)_allLayers.size())
	{
		this->ErrorMessage(tkINVALID_LAYER_HANDLE);
		filename = SysAllocString(L"");
		return filename;
	}

	Layer* layer = _allLayers[layerHandle];
	if (layer  )
	{
		return layer->GetFilename();
	}
	
	filename = SysAllocString(L"");
	return filename;
}

// *********************************************************
//		RemoveLayerOptions()
// *********************************************************
VARIANT_BOOL CMapView::RemoveLayerOptions(LONG LayerHandle, LPCTSTR OptionsName)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CString name = get_OptionsFilename(LayerHandle, OptionsName);
	if (Utility::FileExists(name))
	{
		if( remove( name ) != 0 )
		{
			ErrorMessage(tkCANT_DELETE_FILE);
			return VARIANT_FALSE;
		}
		else
		{
			return VARIANT_TRUE;
		}
	}
	else
	{
		ErrorMessage(tkINVALID_FILENAME);
		return VARIANT_FALSE;
	}
}

// *********************************************************
//		get_OptionsFilename()
// *********************************************************
CString CMapView::get_OptionsFilename(LONG LayerHandle, LPCTSTR OptionsName)
{
	CComBSTR filename;
	filename.Attach(this->GetLayerFilename(LayerHandle));
	
	USES_CONVERSION;
	CString name = OLE2CA(filename);

	// constructing name
	CString dot = (_stricmp(OptionsName, "") == 0) ? "" : ".";
	name += dot;
	name += OptionsName;
	name += ".mwsymb";
	return name;
}

// *********************************************************
//		SaveLayerOptions()
// *********************************************************
VARIANT_BOOL CMapView::SaveLayerOptions(LONG LayerHandle, LPCTSTR OptionsName, VARIANT_BOOL Overwrite, LPCTSTR Description)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	bool result = false;

	Layer* layer = GetLayer(LayerHandle);
	if (layer) 
	{
		if (layer->IsDiskBased())
		{
			CString name = get_OptionsFilename(LayerHandle, OptionsName);

			if (Utility::FileExists(name))
			{
				if (!Overwrite)
				{
					ErrorMessage(tkCANT_CREATE_FILE);
					return VARIANT_FALSE;
				}
				else
				{
					if (remove(name) != 0)
					{
						ErrorMessage(tkCANT_DELETE_FILE);
						return VARIANT_FALSE;
					}
				}
			}

			CPLXMLNode* psTree = LayerOptionsToXmlTree(LayerHandle);
			if (psTree)
			{
				USES_CONVERSION;
				result = GdalHelper::SerializeXMLTreeToFile(psTree, A2W(name)) != 0;		// TODO: use Unicode
				CPLDestroyXMLNode(psTree);
			}
		}
		else if (layer->IsOgrLayer())
		{
			CPLXMLNode* psTree = LayerOptionsToXmlTree(LayerHandle);
			if (psTree)
			{
				char* buffer = CPLSerializeXMLTree(psTree);
				CPLDestroyXMLNode(psTree);

				if (!buffer) return VARIANT_FALSE;
				CStringW xml = Utility::ConvertFromUtf8(buffer);
				CPLFree(buffer);

				IOgrLayer* ogrLayer = NULL;
				layer->QueryOgrLayer(&ogrLayer);
				if (ogrLayer)
				{
					VARIANT_BOOL vb;
					CComBSTR bstr(OptionsName);
					((COgrLayer*)ogrLayer)->SaveStyle(bstr, xml, &vb);
					if (vb) result = true;
				}
			}
		}
	}
	return result ? VARIANT_TRUE : VARIANT_FALSE;
}

// *********************************************************
//		LayerOptionsToXmlTree()
// *********************************************************
CPLXMLNode* CMapView::LayerOptionsToXmlTree(long layerHandle)
{
	CPLXMLNode* node = this->SerializeLayerCore(layerHandle, "");
	if (node)
	{
		CPLXMLNode* psTree = CPLCreateXMLNode(NULL, CXT_Element, "MapWinGIS");
		if (psTree)
		{
			Utility::WriteXmlHeaderAttributes(psTree, "LayerFile");
			USES_CONVERSION;
			CPLAddXMLChild(psTree, node);
			return psTree;
		}
		CPLDestroyXMLNode(node);
	}
	return NULL;
}

// *********************************************************
//		LoadLayerOptionsCore()
// *********************************************************
VARIANT_BOOL CMapView::LoadLayerOptionsCore(CString baseName, LONG LayerHandle, LPCTSTR OptionsName, BSTR* Description)
{
	if (_stricmp(baseName, "") == 0) 
	{
		return VARIANT_FALSE;		// error code is in the function
	}

	CString name;
	if (_stricmp(OptionsName, "") == 0) 
	{
		OptionsName = "default";
	}
	
	// shp.view-default.mwsymb
	name = baseName;
	name += ".";  //view-";
	name += OptionsName;
	name += ".mwsymb";
	
	if (!Utility::FileExists(name))
	{
		// shp.mwsymb
		name = baseName + ".mwsymb";
		if (!Utility::FileExists(name))
		{
			ErrorMessage(tkINVALID_FILENAME);
			return VARIANT_FALSE;
		}
	}

	VARIANT_BOOL success = VARIANT_FALSE;
	CPLXMLNode* root = CPLParseXMLFile(name);		// TODO: use Unicode
	if (root)
	{
		if (_stricmp(root->pszValue, "MapWinGIS") != 0)
		{
			ErrorMessage(tkINVALID_FILE);
		}
		else
		{
			CString s = CPLGetXMLValue(root, "Description", NULL);
			*Description = A2BSTR(s);
			CPLXMLNode* node = CPLGetXMLNode(root, "Layer");
			success = DeserializeLayerOptionsCore(LayerHandle, node);
		}
		CPLDestroyXMLNode(root);
	}
	return success;
}

// *********************************************************
//		LoadLayerOptions()
// *********************************************************
VARIANT_BOOL CMapView::LoadLayerOptions(LONG LayerHandle, LPCTSTR OptionsName, BSTR* Description)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	Layer* l = GetLayer(LayerHandle);
	if (l)
	{
		if (l->IsOgrLayer())
		{
			USES_CONVERSION;
			return LoadOgrStyle(l, LayerHandle, A2W(OptionsName), true);
		}
	}

	CComBSTR filename;
	filename.Attach(this->GetLayerFilename(LayerHandle));
	
	// constructing name
	USES_CONVERSION;
	CString baseName = OLE2CA(filename);

	return LoadLayerOptionsCore(baseName, LayerHandle, OptionsName, Description);
}
#pragma endregion

// *******************************************************
//		GetLayerSkipOnSaving
// *******************************************************
VARIANT_BOOL CMapView::GetLayerSkipOnSaving(LONG LayerHandle)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	if (LayerHandle < 0 || LayerHandle >= (long)_allLayers.size())
	{
		this->ErrorMessage(tkINVALID_LAYER_HANDLE);
		return VARIANT_FALSE;
	}
	
	Layer* layer = _allLayers[LayerHandle];
	if (layer)
	{
		return layer->skipOnSaving;
	}
	else
	{
		return VARIANT_FALSE;
	}
}

// *******************************************************
//		SetLayerSkipOnSaving
// *******************************************************
void CMapView::SetLayerSkipOnSaving(LONG LayerHandle, VARIANT_BOOL newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (LayerHandle < 0 || LayerHandle >= (long)_allLayers.size())
	{
		this->ErrorMessage(tkINVALID_LAYER_HANDLE);
		return;
	}

	Layer* layer = _allLayers[LayerHandle];
	if (layer)
	{
		layer->skipOnSaving = newVal;
	}
}

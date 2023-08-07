#include "../Lib.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ARC_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Inventory.h"
#include "../Player.h"
#include "../Weapon.h"


/*
===============================================================================

	anItemPoolEntry

===============================================================================
*/

/*
==============
anItemPoolEntry::anItemPoolEntry
==============
*/
anItemPoolEntry::anItemPoolEntry( void ) {
	flags.disabled	= false;
	flags.hidden	= false;
	item			= nullptr;
	joint			= INVALID_JOINT;
}

/*
==============
anItemPoolEntry::SetItem
==============
*/
void anItemPoolEntry::SetItem( const anItem *_item ) {
	item = _item;

	if ( item ) {
		for ( int j = 0; j < item->GetClips().Num(); j++ ) {
			clips.Alloc() = -1;
		}
	} else {
		clips.SetNum( 0, false );
	}
}

/*
===============================================================================

	anPlayerClassSetup

===============================================================================
*/

/*
==============
anPlayerClassSetup::anPlayerClassSetup
==============
*/
anPlayerClassSetup::anPlayerClassSetup( void ) : playerClass( nullptr ) {
}

/*
==============
anPlayerClassSetup::operator=
==============
*/
void anPlayerClassSetup::operator=( const anPlayerClassSetup& rhs ) {
	playerClass = rhs.playerClass;
	playerClassOptions.SetNum( rhs.playerClassOptions.Num(), false );
	for ( int i = 0; i < playerClassOptions.Num(); i++ ) {
		playerClassOptions[ i ] = rhs.playerClassOptions[ i ];
	}
}

/*
==============
anPlayerClassSetup::SetOptions
==============
*/
void anPlayerClassSetup::SetOptions( const anList< int >& options ) {
	if ( options.Num() != playerClassOptions.Num() ) {
		gameLocal.Warning( "anPlayerClassSetup::SetOptions Number of options did not match" );
		return;
	}

	playerClassOptions = options;
}

/*
==============
anPlayerClassSetup::SetClass
==============
*/
bool anPlayerClassSetup::SetClass( const anDeclPlayerClass* pc, bool force ) {
	if ( pc == playerClass && !force ) {
		return false;
	}

	playerClass = pc;
	if ( playerClass ) {
		playerClassOptions.Fill( playerClass->GetNumOptions(), 0 );
	} else {
		playerClassOptions.SetNum( 0, false );
	}

	return true;
}

/*
==============
anPlayerClassSetup::SetOption
==============
*/
bool anPlayerClassSetup::SetOption( int index, int itemIndex ) {
	if ( index < 0 || index >= playerClassOptions.Num() ) {
		return false;
	}

	if ( playerClassOptions[ index ] == itemIndex ) {
		return false;
	}

	playerClassOptions[ index ] = itemIndex;
	return true;
}


/*
============
anPlayerClassSetup::GetOption
============
*/
int	anPlayerClassSetup::GetOption( int index ) const {
	assert( index >= 0 && index < playerClassOptions.Num() );

	return playerClassOptions[ index ];
}

/*
===============================================================================

	anUpgradeItemPool

===============================================================================
*/

/*
==============
anUpgradeItemPool::OnHide
==============
*/
void anUpgradeItemPool::OnHide( void ) {
	for ( int i = 0; i < modelItems.Num(); i++ ) {
		modelItems[ i ]->HideModel();
	}
	modelItems.SetNum( 0, false );
}

/*
==============
anUpgradeItemPool::OnShow
==============
*/
void anUpgradeItemPool::OnShow( void ) {
	for ( int i = 0; i < items.Num(); i++ ) {
		if ( !items[ i ].IsVisible() ) {
			continue;
		}
		ShowItem( items[ i ] );
	}
}

/*
==============
anUpgradeItemPool::ShowItem
==============
*/
bool anUpgradeItemPool::ShowItem( anItemPoolEntry& item ) {
	if ( !item.GetModel().hModel ) {
		return false;
	}

	modelItems.AddUnique( &item );
	item.ShowModel();

	return true;
}

/*
==============
anUpgradeItemPool::HideItem
==============
*/
void anUpgradeItemPool::HideItem( anItemPoolEntry& item ) {
	modelItems.RemoveFast( &item );
	item.HideModel();
}

/*
==============
anUpgradeItemPool::AddItem
==============
*/
int anUpgradeItemPool::AddItem( const anInventoryItem* item, bool enabled ) {
	anItemPoolEntry& entry = items.Alloc();

	entry.SetItem( item );
	entry.SetDisabled( !enabled );

	anBasePlayer* player = parent->GetOwner();

	anRenderModel* model = item->GetModel();
	if ( model && enabled ) {
		renderEntity_t& entity = entry.GetModel();

		entity.spawnID						= gameLocal.GetSpawnId( player );//->entityNumber;
		entity.suppressSurfaceInViewID		= player->entityNumber + 1;
		entity.noSelfShadowInViewID			= player->entityNumber + 1;
		entity.axis.Identity();
		entity.origin.Zero();
		entity.hModel						= model;
		entity.bounds						= entity.hModel->Bounds( &entity );
		entity.maxVisDist					= item->GetData().GetInt( "maxVisDist", "2048" );

		SetupModelShadows( entry );
		ShowItem( entry );
	}

	entry.joint = player->GetAnimator()->GetJointHandle( item->GetJoint() );

	return items.Num() - 1;
}

/*
==============
anUpgradeItemPool::ClearItem
==============
*/
void anUpgradeItemPool::ClearItem( anItemPoolEntry& entry ) {
	HideItem( entry );
	entry.SetItem( nullptr );
}

/*
==============
anUpgradeItemPool::Init
==============
*/
void anUpgradeItemPool::Init( anInventory* _parent ) {
	parent = _parent;
}

/*
==============
anUpgradeItemPool::Clear
==============
*/
void anUpgradeItemPool::Clear( void ) {
	for( int i = 0; i < items.Num(); i++ ) {
		ClearItem( items[ i ] );
	}
	items.Clear();
}

/*
==============
anUpgradeItemPool::ApplyPlayerState
==============
*/
void anUpgradeItemPool::ApplyPlayerState( const anInventoryPlayerStateData& newState ) {	NET_GET_NEW( anInventoryPlayerStateData );
	for( int i = 0; i < items.Num(); i++ ) {
		anItemPoolEntry &item = items[ i ];
		for ( int j = 0; j < item.clips.Num(); j++ ) {
			if ( item.GetItem()->GetClips()[ j ].maxAmmo <= 0 ) {
				continue;
			}

			item.clips[ j ] = newData.itemData[ i ].clips[ j ];
		}
	}
}

/*
==============
anUpgradeItemPool::ReadPlayerState
==============
*/
void anUpgradeItemPool::ReadPlayerState( const anInventoryPlayerStateData& baseState, anInventoryPlayerStateData& newState, const anBitMsg& msg ) const {
	newData.itemData.SetNum( items.Num() );

	for( int i = 0; i < items.Num(); i++ ) {
		const anItemPoolEntry& item = items[ i ];
		newData.itemData[ i ].clips.SetNum( item.clips.Num() );
		for ( int j = 0; j < item.clips.Num(); j++ ) {
			if ( i < baseData.itemData.Num() && j < baseData.itemData[ i ].clips.Num() ) {
				newData.itemData[ i ].clips[ j ] = msg.ReadDeltaShort( baseData.itemData[ i ].clips[ j ] );
			} else {
				newData.itemData[ i ].clips[ j ] = msg.ReadShort();
			}
		}
	}
}

/*
==============
anUpgradeItemPool::WritePlayerState
==============
*/
void anUpgradeItemPool::WritePlayerState( const anInventoryPlayerStateData &baseState, anInventoryPlayerStateData &newState, anBitMsg &msg ) const {
	newData.itemData.SetNum( items.Num() );

	for( int i = 0; i < items.Num(); i++ ) {
		const anItemPoolEntry& item = items[ i ];
		newData.itemData[ i ].clips.SetNum( item.clips.Num() );
		for ( int j = 0; j < item.clips.Num(); j++ ) {
			newData.itemData[ i ].clips[ j ] = item.clips[ j ];
			if ( i < baseData.itemData.Num() && j < baseData.itemData[ i ].clips.Num() ) {
				msg.WriteDeltaShort( baseData.itemData[ i ].clips[ j ], newData.itemData[ i ].clips[ j ] );
			} else {
				msg.WriteShort( newData.itemData[ i ].clips[ j ] );
			}
		}
	}
}

/*
==============
anUpgradeItemPool::CheckPlayerStateChanges
==============
*/
bool anUpgradeItemPool::CheckPlayerStateChanges( const anInventoryPlayerStateData& baseState ) const {
	if ( baseData.itemData.Num() != items.Num() ) {
		return true;
	}
	for( int i = 0; i < items.Num(); i++ ) {
		const anItemPoolEntry& item = items[ i ];
		if ( baseData.itemData[ i ].clips.Num() != item.clips.Num() ) {
			return true;
		}

		for ( int j = 0; j < item.clips.Num(); j++ ) {
			if ( baseData.itemData[ i ].clips[ j ] != item.clips[ j ] ) {
				return true;
			}
		}
	}

	return false;
}

/*
==============
anUpgradeItemPool::UpdateJoints
==============
*/
void anUpgradeItemPool::UpdateJoints( void ) {
	anBasePlayer *owner = parent->GetOwner();

	for( int i = 0; i < items.Num(); i++ ) {
		items[ i ].joint = owner->GetAnimator()->GetJointHandle( items[ i ].item->GetJoint() );
	}
}

/*
==============
anUpgradeItemPool::UpdateModels
==============
*/
void anUpgradeItemPool::UpdateModels( void ) {
	// no need to do this in reprediction
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	anBasePlayer *owner = parent->GetOwner();

	for ( int i = 0; i < modelItems.Num(); i++ ) {
		anItemPoolEntry& item = *modelItems[ i ];
		renderEntity_t &entity = item.GetModel();

		entity.customSkin = owner->GetRenderEntity()->customSkin;
		owner->GetWorldOriginAxisNoUpdate( item.joint, entity.origin, entity.axis );

		item.UpdateModel();
	}
}

/*
==============
anUpgradeItemPool::~anUpgradeItemPool
==============
*/
anUpgradeItemPool::~anUpgradeItemPool( void ) {
	Clear();
}

/*
==============
anUpgradeItemPool::SetupModelShadows
==============
*/
void anUpgradeItemPool::SetupModelShadows( anItemPoolEntry& entry ) {
	anBasePlayer *owner = parent->GetOwner();

	renderEntity_t &renderEntity = entry.GetModel();

	viewState_t state = owner->HasShadow();
	switch ( state ) {
		case VS_NONE: {
			renderEntity.suppressShadowInViewID = 0;
			renderEntity.flags.noShadow			= true;
			break;
		}
		case VS_REMOTE: {
			renderEntity.suppressShadowInViewID = owner->entityNumber + 1;
			renderEntity.flags.noShadow			= false;
			break;
		}
		case VS_FULL: {
			renderEntity.suppressShadowInViewID = 0;
			renderEntity.flags.noShadow			= false;
			break;
		}
	}
}

/*
============
anUpgradeItemPool::UpdateModelShadows
============
*/
void anUpgradeItemPool::UpdateModelShadows( void ) {
	for ( int i = 0; i < modelItems.Num(); i++ ) {
		SetupModelShadows( *modelItems[ i ] );
	}
}

/*
===============================================================================

	anInventory

===============================================================================
*/

anList<int> anInventory::slotForBank;

/*
==============
anInventoryBroadcastData::MakeDefault
==============
*/
void anInventoryBroadcastData::MakeDefault( void ) {
	idealWeapon = -1;
	disabledMask.Shutdown();
	ammoMask.Shutdown();

	proficiencyData.MakeDefault();
}

/*
==============
anInventoryBroadcastData::Write
==============
*/
void anInventoryBroadcastData::Write( anFile *file ) const {
	file->WriteInt( idealWeapon );
	file->WriteInt( disabledMask.GetSize() );
	for ( int i = 0; i < disabledMask.GetSize(); i++ ) {
		file->WriteInt( disabledMask.GetDirect( i ) );
	}

	file->WriteInt( ammoMask.GetSize() );
	for ( int i = 0; i < ammoMask.GetSize(); i++ ) {
		file->WriteInt( ammoMask.GetDirect( i ) );
	}

	proficiencyData.Write( file );
}

/*
==============
anInventoryBroadcastData::Read
==============
*/
void anInventoryBroadcastData::Read( anFile *file ) {
	file->ReadInt( idealWeapon );

	int sizeDummy;

	file->ReadInt( sizeDummy );
	disabledMask.SetSize( sizeDummy );
	for ( int i = 0; i < disabledMask.GetSize(); i++ ) {
		file->ReadInt( disabledMask.GetDirect( i ) );
	}

	file->ReadInt( sizeDummy );
	ammoMask.SetSize( sizeDummy );
	for ( int i = 0; i < ammoMask.GetSize(); i++ ) {
		file->ReadInt( ammoMask.GetDirect( i ) );
	}

	proficiencyData.Read( file );
}

/*
==============
anInventoryPlayerStateData::MakeDefault
==============
*/
void anInventoryPlayerStateData::MakeDefault( void ) {
	ammo.SetNum( gameLocal.declAmmoTypeType.Num() );
	for ( int i = 0; i < ammo.Num(); i++ ) {
		ammo[ i ] = 0;
	}
	itemData.SetNum( 0, false );
}

/*
==============
anInventoryPlayerStateData::Write
==============
*/
void anInventoryPlayerStateData::Write( anFile *file ) const {
	file->WriteInt( ammo.Num() );
	for ( int i = 0; i < ammo.Num(); i++ ) {
		file->WriteShort( ammo[ i ] );
	}

	file->WriteInt( itemData.Num() );
	for ( int i = 0; i < itemData.Num(); i++ ) {
		itemData[ i ].Write( file );
	}
}

/*
==============
anInventoryPlayerStateData::Read
==============
*/
void anInventoryPlayerStateData::Read( anFile *file ) {
	int count;
	file->ReadInt( count );

	assert( gameLocal.declAmmoTypeType.Num() == count );

	ammo.SetNum( count );
	for ( int i = 0; i < count; i++ ) {
		file->ReadShort( ammo[ i ] );
	}

	file->ReadInt( count );

	itemData.SetNum( count );
	for ( int i = 0; i < count; i++ ) {
		itemData[ i ].Read( file );
	}
}

/*
==============
anInventoryItemStateData::MakeDefault
==============
*/
void anInventoryItemStateData::MakeDefault( void ) {
	clips.SetNum( 0, false );
}

/*
==============
anInventoryItemStateData::Write
==============
*/
void anInventoryItemStateData::Write( anFile *file ) const {
	file->WriteInt( clips.Num() );
	for ( int i = 0; i < clips.Num(); i++ ) {
		file->WriteShort( clips[ i ] );
	}
}

/*
==============
anInventoryItemStateData::Read
==============
*/
void anInventoryItemStateData::Read( anFile *file ) {
	int count;
	file->ReadInt( count );

	clips.SetNum( count );
	for ( int i = 0; i < count; i++ ) {
		file->ReadShort( clips[ i ] );
	}
}

/*
==============
anInventory::GetCommandMapIcon
==============
*/
const anMaterial *anInventory::GetCommandMapIcon( const iconType_e iconType ) const {
	const anDeclPlayerClass* pc = playerClass.GetClass();

	if ( !pc ) {
		return nullptr;
	}

	switch( iconType ) {
		case IT_CLASS:
			return pc->GetCommandmapIconClass();
		case IT_ICON:
			return pc->GetCommandmapIcon();
		case IT_UNKNOWN:
			return pc->GetCommandmapIconUnknown();
	}

	return nullptr;
}

/*
==============
anInventory::UpdateItems
==============
*/
void anInventory::UpdateItems( void ) {
	if ( classThread ) {
		if ( classThread->Execute() ) {
			ShutdownClassThread();
		}
	}
}

/*
==============
anInventory::Present
==============
*/
void anInventory::Present( void ) {
	items.UpdateModels();
}

/*
==============
anInventory::Init
==============
*/
void anInventory::Init( anDict &dict, bool full, bool setWeapon ) {
	ammo.AssureSize( gameLocal.declAmmoTypeType.Num(), 0 );
	ammoLimits.AssureSize( gameLocal.declAmmoTypeType.Num(), 0 );

	ClearAmmo();
	items.Clear();

	currentWeapon	= -1;
	if ( !gameLocal.isClient ) {
		idealWeapon		= -1;
	}
	switchingWeapon	= -1;

	if ( !gameLocal.isClient ) {
		CheckPlayerClass( setWeapon );
	}
	SetupClassOptions( true, setWeapon );

	SetupModel();

	LogClassTime();
}

/*
==============
anInventory::WriteDemoBaseData
==============
*/
void anInventory::WriteDemoBaseData( anFile *file ) const {
	int playerClassIndex = playerClass.GetClass() ? playerClass.GetClass()->Index() : -1;
	file->WriteInt( playerClassIndex );
	if ( playerClassIndex != -1 ) {
		for ( int i = 0; i < playerClass.GetOptions().Num(); i++ ) {
			file->WriteInt( playerClass.GetOptions()[ i ] );
		}
	}

	int cachedPlayerClassIndex = cachedPlayerClass.GetClass() ? cachedPlayerClass.GetClass()->Index() : -1;

	file->WriteInt( cachedPlayerClassIndex );
	if ( cachedPlayerClassIndex != -1 ) {
		for ( int i = 0; i < cachedPlayerClass.GetOptions().Num(); i++ ) {
			file->WriteInt( cachedPlayerClass.GetOptions()[ i ] );
		}
	}
}

/*
==============
anInventory::ReadDemoBaseData
==============
*/
void anInventory::ReadDemoBaseData( anFile *file ) {
	int playerClassIndex;

	file->ReadInt( playerClassIndex );
	if ( playerClassIndex != -1 ) {
		const anDeclPlayerClass* cls = gameLocal.declPlayerClassType[ playerClassIndex ];

		SetPlayerClass( cls );

		for ( int i = 0; i < playerClass.GetOptions().Num(); i++ ) {
			int itemIndex;
			file->ReadInt( itemIndex );
			playerClass.SetOption( i, itemIndex );
		}
	}

	int cachedPlayerClassIndex;

	file->ReadInt( cachedPlayerClassIndex );
	if ( cachedPlayerClassIndex != -1 ) {
		const anDeclPlayerClass* cls = gameLocal.declPlayerClassType[ cachedPlayerClassIndex ];
		cachedPlayerClass.SetClass( cls, true );
		for ( int i = 0; i < cachedPlayerClass.GetOptions().Num(); i++ ) {
			int itemIndex;
			file->ReadInt( itemIndex );
			cachedPlayerClass.SetOption( i, itemIndex );
		}
	}

	SetupClassOptions( true, false );
}

/*
==============
anInventory::anInventory
==============
*/
anInventory::anInventory( void ) {
	weaponChanged		= false;
	classThread			= nullptr;
	timeClassChanged	= 0;
	currentWeaponIndex	= -1;
	idealWeapon			= -1;
	SetOwner( nullptr );
	items.Init( this );
}

/*
==============
anInventory::SetOwner
==============
*/
void anInventory::SetOwner( anBasePlayer* _owner ) {
	owner = _owner;
}

/*
==============
anInventory::GetOwner
==============
*/
anBasePlayer* anInventory::GetOwner( void ) {
	return owner;
}

/*
==============
anInventory::~anInventory
==============
*/
anInventory::~anInventory( void ) {
}

/*
==============
anInventory::GetAmmoFraction
==============
*/
float anInventory::GetAmmoFraction( void ) {
	if ( !playerClass.GetClass() ) {
		return 0.f;
	}

	int max = 0;
	int ammoCount = 0;
	for ( int i = 0; i < ammo.Num(); i++ ) {
		ammoCount += ammo[ i ];
		max += ammoLimits[ i ];
	}

	return ammoCount / ( float )( max );
}

/*
==============
anInventory::BuildSlotBankLookup
==============
*/
void anInventory::BuildSlotBankLookup( void ) {
	slotForBank.Clear();

	const sdDeclStringMap* stringMap = gameLocal.declStringMapType.LocalFind( "inventorySlots" );
	const anDict &dict = stringMap->GetDict();

	int i = 0;
	while ( true ) {
		//const sdDeclInvSlot* slot = gameLocal.declInvSlotType.LocalFindByIndex( i, true );
		const char *value = dict.GetString( va( "slot%i", i ) );
		i++;
		if ( value[0] == '\0' ) {
			break;
		}
		const sdDeclInvSlot* slot = gameLocal.declInvSlotType.LocalFind( value, true );

		int slotBank = slot->GetBank();
		if ( slotBank == -1 ) {
			continue;
		}

		slotForBank.AssureSize( slotBank + 1, -1 );

		if ( slotForBank[ slotBank ] != -1 ) {
			gameLocal.Error( "anInventory::BuildSlotBankLookup Multiple Slots Using Weapon Bank %i", slotBank );
		}

		slotForBank[ slotBank ] = slot->Index();		
	}
}

/*
==============
anInventory::GetSlotForWeapon
==============
*/
int anInventory::GetSlotForWeapon( int weapon ) const {
	if ( weapon >= 0 && weapon < items.Num() ) {
		if ( CanEquip( weapon, false ) ) {
			int count = slotForBank.Num();
			for ( int i = count - 1; i >= 0; i-- ) {
				int slot = slotForBank[ i ];
				if ( slot == -1 ) {
					continue;
				}

				if ( items[ weapon ].item->UsesSlot( slot ) ) {
					return i;
				}
			}
		}
	}
	return -1;
}

/*
==============
anInventory::GetCurrentSlot
==============
*/
int anInventory::GetCurrentSlot( void ) const {
	return GetSlotForWeapon( idealWeapon );
}

/*
==============
anInventory::GetSwitchingSlot
==============
*/
int anInventory::GetSwitchingSlot( void ) const {
	return GetSlotForWeapon( switchingWeapon );
}

/*
============
anInventory::FindBestWeapon
============
*/
int anInventory::FindBestWeapon( bool allowCurrent ) {
	int bestIndex = -1;
	int bestRank = 9999;

	for ( int i = 0; i < items.Num(); i++ ) {
		if ( !allowCurrent ) {
			if ( ( i == switchingWeapon || i == currentWeapon ) ) {
				continue;
			}
		}

		if ( !CanAutoEquip( i, true ) ) {
			continue;
		}

		const anInventoryItem* item = items[ i ].item;

		int rank = item->GetAutoSwitchPriority();
		if ( rank < bestRank && rank > 0 && CheckWeaponHasAmmo( item ) ) {
			bestIndex = i;
			bestRank = rank;
		}
	}

	return bestIndex;
}

/*
============
anInventory::CycleNextSafeWeapon
============
*/
void anInventory::SelectBestWeapon( bool allowCurrent ) {	
	int bestIndex = FindBestWeapon( allowCurrent );
	if ( bestIndex != -1 ) {
		SetIdealWeapon( bestIndex );
	}
}

/*
============
anInventory::CycleNextSafeWeapon
============
*/
void anInventory::CycleNextSafeWeapon( void ) {	
	int bestIndex = FindBestWeapon( false );
	if ( bestIndex != -1 ) {
		SetSwitchingWeapon( bestIndex );
	}
}

/*
==============
anInventory::CycleWeaponsNext
==============
*/
void anInventory::CycleWeaponsNext( int currentSlot ) {
	bool force = true;
	bool looped;
	int weapon;

	if ( currentSlot == -999 ) {
		force = false;
		currentSlot = ChooseCurrentSlot();
	}
	
	weapon = CycleWeaponByPosition( currentSlot, true, looped, false, false );

	if ( !looped && weapon != -1 ) {
		SetSwitchingWeapon( weapon );
		return;
	}

	int cnt = 0;
	int max = slotForBank.Num();
	for( int i = currentSlot + 1; cnt < max; cnt++ ) {
		int pos = ( i + cnt ) % max;

		weapon = CycleWeaponByPosition( pos, true, looped, false, false );
		if ( weapon != -1 ) {
			SetSwitchingWeapon( weapon );
			return;
		}
	}
}

/*
==============
anInventory::CycleWeaponsPrev
==============
*/
void anInventory::CycleWeaponsPrev( int currentSlot ) {
	bool force = true;
	bool looped;
	int weapon;

	if ( currentSlot == -999 ) {
		force = false;
		currentSlot = ChooseCurrentSlot();
	}

	weapon = CycleWeaponByPosition( currentSlot, false, looped, false, false );
	if ( !looped && weapon != -1 ) {
		SetSwitchingWeapon( weapon );
		return;
	}

	int cnt = 0;
	int max = slotForBank.Num();
	for( int i = currentSlot - 1; cnt < max; cnt++ ) {
		int pos = ( ( i - cnt ) + max ) % max;

		weapon = CycleWeaponByPosition( pos, false, looped, false, false );
		if ( weapon != -1 ) {
			SetSwitchingWeapon( weapon );
			return;
		}
	}
}

/*
==============
anInventory::CycleWeaponByPosition

if "primaryOnly" == true, it will only select the first weapon in that slot, never looping to any other weapon that
may share that slot.
==============
*/
int anInventory::CycleWeaponByPosition( int pos, bool forward, bool& looped, bool force, bool primaryOnly ) {
	if ( pos < 0 || pos >= slotForBank.Num() ) {
		return false;
	}

	int slot = slotForBank[ pos ];
	if ( slot == -1 ) {
		return false;
	}

	int startpos = 0;

	if ( !primaryOnly ) {
        if ( ( ChooseCurrentSlot() == pos ) ) {
			if ( switchingWeapon >= 0 ) {
				startpos = switchingWeapon;
			} else if ( idealWeapon >= 0 ) {
				startpos = idealWeapon;
			} else {
				startpos = currentWeapon;
			}
		}
	}

	return CycleWeaponBySlot( slot, forward, looped, force, startpos );
}

/*
==============
anInventory::UpdatePrimaryWeapon
==============
*/
void anInventory::UpdatePrimaryWeapon( void ) {
    for ( int i = 0; i < items.Num(); i++ ) {
		const anInventoryItem* item = items[ i ].GetItem();
		if ( item == nullptr || !item->UsesSlot( slotForBank[ GUN_SLOT ] ) ) {
			continue;
		}

		if ( !CanEquip( i, true ) ) {
			continue;
		}

		int weaponNum = item->GetData().GetInt( "player_weapon_num", "-1" );

		if ( weaponNum != 14 && weaponNum != -1 ) { //mal_FIXME: 14 = hack for the AR/Grenade Launcher combo. Need to fix this!
			botThreadData.GetGameWorldState()->clientInfo[owner->entityNumber].weapInfo.primaryWeapon = ( playerWeaponTypes_t ) weaponNum;
			break;
		}
	}
}

/*
==============
anInventory::CheckWeaponSlotHasAmmo
==============
*/
bool anInventory::CheckWeaponSlotHasAmmo( int slot ) {
	bool hasAmmo = false;
	int ammoInClip, clipSize;

    for ( int i = 0; i < items.Num(); i++ ) {
		const anInventoryItem *item = items[ i ].GetItem();
		if ( item == nullptr || !item->UsesSlot( slotForBank[ slot ] ) ) {
			continue;
		}

		if ( !CanEquip( i, true ) ) {
			continue;
		}

		if ( CheckWeaponHasAmmo( item ) ) {
			hasAmmo = true;
			if ( slot == GUN_SLOT ) { //mal: if this is our primary gun we're checking, see if it needs ammo.
				const anList< itemClip_t >& itemClips = item->GetClips();
				int ammoFraction;
				int maxAmmo, curAmmo;
				if ( !itemClips.Num() ) {
					break;
				}

				if ( client.team == GDF ) {
					if ( client.classType == SOLDIER && client.weapInfo.primaryWeapon == SMG ) {
                        ammoFraction = 4; //mal: GDF soldiers with assault rifle get as much ammo as strogg!
					} else {
						ammoFraction = 2;
					}
				} else {
					if ( client.weapInfo.primaryWeapon == ROCKET ) {
						ammoFraction = 2;
					} else {
						ammoFraction = 4; //mal: non-rocket launcher strogg have SO much more ammo then GDF.
					}
				}

				maxAmmo = GetMaxAmmo( itemClips[ MAIN_GUN ].ammoType );
				curAmmo = GetAmmo( itemClips[ MAIN_GUN ].ammoType );
				if ( client.team == STROGG && client.weapInfo.primaryWeapon != ROCKET ) {
					client.weapInfo.primaryWeapNeedsReload = false;
					client.weapInfo.primaryWeapClipEmpty = false;
				} else {
					ammoInClip = GetClip( i, MAIN_GUN );
					clipSize = GetClipSize( i, MAIN_GUN );
					if ( ammoInClip == 0 || ammoInClip < clipSize ) {
						client.weapInfo.primaryWeapNeedsReload = true;
					} else {
						client.weapInfo.primaryWeapNeedsReload = false;
					}

					if ( ammoInClip == 0 ) {
						client.weapInfo.primaryWeapClipEmpty = true;
					} else {
						client.weapInfo.primaryWeapClipEmpty = false;
					}

					if ( curAmmo > clipSize ) {
						client.weapInfo.hasAmmoForReload = true;
					} else {
						client.weapInfo.hasAmmoForReload = false;
					}
				}

				if ( ( maxAmmo / ammoFraction ) > curAmmo ) {
					botThreadData.GetGameWorldState()->clientInfo[owner->entityNumber].weapInfo.primaryWeapNeedsAmmo = true;
				} else {
					botThreadData.GetGameWorldState()->clientInfo[owner->entityNumber].weapInfo.primaryWeapNeedsAmmo = false;
				}
			}
			break;
		}
	}

	return hasAmmo;
}

/*
==============
anInventory::CyleWeaponBySlot
==============
*/
int anInventory::CycleWeaponBySlot( int slot, bool forward, bool& looped, bool force, int startingpos ) {
	looped = false;

	int i = startingpos;// + ( forward ? 1 : -1 );
	int j;
	int max = items.Num();

	for( j = 0; j < max; j++ ) {
		if ( forward ) {
			i++;
			if ( i >= max ) { looped = true;	i -= max;}
		} else {
			i--;
			if ( i < 0 ) { looped = true; i += max; }
		}

//		if ( !force ) {
//
//			if ( /*!looped && */( i == switchingWeapon || i == currentWeapon )) {
//				continue;
//			}
//		}

		if ( !CanAutoEquip( i, false ) ) {
			continue;
		}

		const anInventoryItem* item = items[ i ].item;
		if ( !item->UsesSlot( slot ) ) {
			continue;
		}

		if ( !item->GetSelectWhenEmpty() && !CheckWeaponHasAmmo( item )) {
			continue;
		}

		return i;
	}

	return -1;
}

/*
==============
anInventory::SelectWeaponByName
==============
*/
void anInventory::SelectWeaponByName( const char *weaponName, bool ignoreInhibit ) {
	if ( !ignoreInhibit  ) {
		if ( owner->InhibitWeaponSwitch() ) {
			return;
		}
	}

	int weaponIndex = FindWeapon( weaponName );
	if ( weaponIndex == -1 ) {
		return;
	}

	if ( !CanEquip( weaponIndex, true ) ) {
		return;
	}

	SetIdealWeapon( weaponIndex );
}

/*
==============
anInventory::SelectWeaponByNumber
==============
*/
void anInventory::SelectWeaponByNumber( const playerWeaponTypes_t weaponNum ) {
	if ( owner->InhibitWeaponSwitch() ) {
		return;
	}

	int weaponIndex = FindWeaponNum( weaponNum );
	if ( weaponIndex == -1 ) {
		return;
	}

	if ( !CanEquip( weaponIndex, true ) ) {
		return;
	}

	SetIdealWeapon( weaponIndex );
}

/*
==============
anInventory::CanEquip
==============
*/
bool anInventory::CanEquip( int index, bool checkRequirements ) const {
	if ( index < 0 || index >= items.Num() ) {
		return false;
	}

	const anInventoryItem* item = items[ index ].item;
	if ( item == nullptr ) {
		assert( false );
		return false;
	}

	if ( items[ index ].IsDisabled() ) {
		return false;
	}

	if ( owner->GetProxyEntity() != nullptr ) {
		if ( !item->GetType()->IsVehicleEquipable() ) {
			return false;
		}
	} else {
		if ( !item->GetType()->IsEquipable() ) {
			return false;
		}
	}

	if ( checkRequirements ) {
		if ( !item->GetUsageRequirements().Check( owner ) ) {
			return false;
		}
	}

	return true;
}

/*
==============
anInventory::CanAutoEquip
==============
*/
bool anInventory::CanAutoEquip( int index, bool checkExplosive ) const {
	if ( !CanEquip( index, true ) ) {
		return false;
	}

	const anInventoryItem* item = items[ index ].item;
	if ( item->GetWeaponMenuIgnore() ) {
		return false;
	}

	if ( checkExplosive ) {
		if ( item->GetAutoSwitchIsExplosive() && owner->userInfo.ignoreExplosiveWeapons ) {
			return false;
		}
	}

	return true;
}

/*
==============
anInventory::FindWeapon
==============
*/
int anInventory::FindWeapon( const char *weaponName ) {
	for( int i = 0; i < items.Num(); i++ ) {
		if ( !CanEquip( i, true ) ) {
			continue;
		}

		const anInventoryItem* item = items[ i ].item;
		if ( !anStr::Icmp( item->GetName(), weaponName ) ) {
			return i;
		}
	}
	return -1;
}

/*
==============
anInventory::FindWeaponNum

A handy function that lets the player pick a specific weapon, without cycling thru the slots.
==============
*/
int anInventory::FindWeaponNum( const playerWeaponTypes_t weaponNum ) {
	for( int i = 0; i < items.Num(); i++ ) {
		if ( !CanEquip( i, true ) ) {
			continue;
		}

		const anInventoryItem* weapItem = items[ i ].item;

		if ( weapItem->GetData().GetInt( "player_weapon_num", "-1" ) == weaponNum ) {
			return i;
		}
	}
	return -1;
}

/*
==============
anInventory::SetIdealWeapon
==============
*/
void anInventory::SetIdealWeapon( int pos, bool force ) {
	if ( pos < -1 || pos >= items.Num() ) {
		return;
	}

	bool set = force || pos == -1;
	if ( !set ) {
		if ( pos == currentWeapon ) {
			set = items[ pos ].item->Index() != currentWeaponIndex;
		} else if ( pos != idealWeapon ) {
			set = true;
		}
	}

	if ( set ) {
		idealWeapon = pos;
		weaponChanged = true;

		if ( pos != -1 ) {		
			currentWeaponIndex = items[ pos ].item->Index();
			owner->SpawnToolTip( items[ pos ].item->GetToolTip() );
		} else {
			currentWeaponIndex = -1;
		}
	}

	CancelWeaponSwitch();
}

/*
==============
anInventory::GetClip
==============
*/
int	anInventory::GetClip( int index, int modIndex ) const {
	if ( index >= items.Num() || modIndex >= items[ index ].clips.Num() ) {
		return 0;
	}
	return items[ index ].clips[ modIndex ];
}


/*
============
anInventory::GetClipSize
============
*/
int	anInventory::GetClipSize( int index, int modIndex ) const {
	if ( index >= items.Num() || modIndex >= items[ index ].clips.Num() ) {
		return 0;
	}
	return items[ index ].GetItem()->GetClips()[ modIndex ].maxAmmo;
}

/*
==============
anInventory::SetClip
==============
*/
void anInventory::SetClip( int index, int modIndex, int count ) {
	if ( index >= items.Num() || modIndex >= items[ index ].clips.Num() ) {
		return;
	}
	items[ index ].clips[ modIndex ] = count;
}

/*
==============
anInventory::IsWeaponValid
==============
*/
bool anInventory::IsWeaponValid( int weapon ) const {
	if ( weapon < 0 || weapon >= items.Num() ) {
		return false;
	}

	return CanEquip( weapon, false );
}

/*
==============
anInventory::IsWeaponBankValid
0-based weapon bank (0 is fists, 1 is pistol, etc)
==============
*/
bool anInventory::IsWeaponBankValid( int slot ) const {
	if ( slot < 0 || slot >= slotForBank.Num() ) {
		return false;
	}
	
	bool allValid = false;
	for ( int i = 0; i < items.Num(); i++ ) {
		if ( !CanEquip( i, true ) ) {
			continue;
		}

		const anInventoryItem* item = items[ i ].GetItem();
		if ( !item->UsesSlot( slotForBank[ slot ] )) {
			continue;
		}

		if ( CheckWeaponHasAmmo( item ) || item->GetSelectWhenEmpty() ) {
			allValid = true;
			break;
		}
	}

	return allValid;
}


/*
============
anInventory::NumValidWeaponBanks
============
*/
int anInventory::NumValidWeaponBanks() const {
	int numValid = 0;
	int max = slotForBank.Num();
	for( int i = 0; i < max; i++ ) {
		if ( IsWeaponBankValid( i )) {
			numValid++;
		}
	}

	return numValid;
}

/*
==============
anInventory::GetActivePlayer
==============
*/
anBasePlayer* anInventory::GetActivePlayer( void ) const {
	anBasePlayer* modelPlayer = nullptr;

	int disguiseClient = owner->GetDisguiseClient();
	if ( disguiseClient != -1 ) {
		modelPlayer = gameLocal.EntityForSpawnId( disguiseClient )->Cast< anBasePlayer >();
	}
	if ( modelPlayer == nullptr ) {
		modelPlayer = owner;
	}
	return modelPlayer;
}

/*
==============
anInventory::GetActivePlayerClass
==============
*/
const anDeclPlayerClass* anInventory::GetActivePlayerClass( void ) const {
	if ( owner->IsDisguised() ) {
		return owner->GetDisguiseClass();
	}
	return playerClass.GetClass();
}

/*
==============
anInventory::SetupModel
==============
*/
void anInventory::SetupModel( void ) {
	SetupModel( GetActivePlayerClass() );
}

/*
==============
anInventory::GetPlayerJoint
==============
*/
jointHandle_t anInventory::GetPlayerJoint( const anDict &dict, const char *name ) {
	const char *value = dict.GetString( name );
	jointHandle_t handle = owner->GetAnimator()->GetJointHandle( value );
	if ( handle == INVALID_JOINT ) {
		gameLocal.Error( "anInventory::GetPlayerJoint '%s' not found for '%s' on '%s'", value, name, owner->name.c_str() );
	}
	return handle;
}

/*
==============
anInventory::SkinForClass
==============
*/
const idDeclSkin* anInventory::SkinForClass( const anDeclPlayerClass* cls ) {
	if ( gameLocal.mapSkinPool != nullptr ) {
		const char *skinKey = cls->GetClimateSkinKey();
		if ( *skinKey != '\0' ) {
			const char *skinName = gameLocal.mapSkinPool->GetDict().GetString( va( "skin_%s", skinKey ) );
			if ( *skinName == '\0' ) {
				gameLocal.Warning( "anInventory::SetupModel No Skin Set For '%s'", skinKey );
			} else {
				const idDeclSkin* skin = gameLocal.declSkinType[ skinName ];
				if ( skin == nullptr ) {
					gameLocal.Warning( "sdScriptEntity::Spawn Skin '%s' Not Found", skinName );
				} else {
					return skin;
				}
			}
		}
	}

	return nullptr;
}

/*
==============
anInventory::SetupModel
==============
*/
void anInventory::SetupModel( const anDeclPlayerClass* cls ) {
	owner->SetSkin( nullptr );

	if ( cls == nullptr ) {
		owner->SetModel( "" );
		owner->UpdateShadows();
		owner->SetHipJoint( INVALID_JOINT );
		owner->SetTorsoJoint( INVALID_JOINT );
		owner->SetHeadJoint( INVALID_JOINT );

		items.UpdateJoints();

		return;
	}

	if ( !cls->GetModel() ) {
		gameLocal.Error( "anInventory::SetupModel nullptr model for class '%s'", cls->GetName() );
	}

	const anDict &dict = cls->GetModelData();
	
	owner->SetModel( cls->GetModel()->GetName() );
	owner->UpdateShadows();

	const idDeclSkin* skin = SkinForClass( cls );
	if ( skin != nullptr ) {
		owner->SetSkin( skin );
	}

	owner->SetHipJoint( GetPlayerJoint( dict, "bone_hips" ) );
	owner->SetHeadJoint( GetPlayerJoint( dict, "bone_head" ) );
	owner->SetChestJoint( GetPlayerJoint( dict, "bone_chest" ) );
	owner->SetTorsoJoint( GetPlayerJoint( dict, "bone_torso" ) );
	owner->SetShoulderJoint( 0, GetPlayerJoint( dict, "bone_left_shoulder" ) );
	owner->SetElbowJoint( 0, GetPlayerJoint( dict, "bone_left_elbow" ) );
	owner->SetHandJoint( 0, GetPlayerJoint( dict, "bone_left_hand" ) );
	owner->SetFootJoint( 0, GetPlayerJoint( dict, "bone_left_foot" ) );
	owner->SetShoulderJoint( 1, GetPlayerJoint( dict, "bone_right_shoulder" ) );
	owner->SetElbowJoint( 1, GetPlayerJoint( dict, "bone_right_elbow" ) );
	owner->SetHandJoint( 1, GetPlayerJoint( dict, "bone_right_hand" ) );
	owner->SetFootJoint( 1, GetPlayerJoint( dict, "bone_right_foot" ) );

	owner->SetHeadModelJoint( GetPlayerJoint( dict, "bone_head_model" ) );
	owner->SetHeadModelOffset( dict.GetVector( "head_offset" ) );

	SetupLocationalDamage( dict );

	idScriptHelper h;
	owner->CallNonBlockingScriptEvent( owner->GetScriptFunction( "OnNewModel" ), h );

	items.UpdateJoints();
}

/*
============
anInventory::SetupLocationalDamage
============
*/
void anInventory::SetupLocationalDamage( const anDict &dict ) {
	owner->RemoveLocationalDamageInfo();

	locationalDamageInfo_t info;
	int	numLocationDamageJoints = dict.GetInt( "loc_damage_joint_num", "6" );
	for( int i = 0; i < numLocationDamageJoints; i++ ) {
		const char *jointName = dict.GetString( va( "loc_damage_joint_%d", i ), "" );
		info.joint = owner->GetAnimator()->GetJointHandle( jointName );
		if ( info.joint == INVALID_JOINT ) {
			gameLocal.Warning( "Invalid locational damage joint %d", i );
			continue;
		}
		
		if ( !owner->GetAnimator()->GetJointTransform( info.joint, gameLocal.time, info.pos ) ) {
			gameLocal.Warning( "Invalid local transform for locational damage joint %d", i );
			continue;
		}

		info.area = owner->LocationalDamageAreaForString( dict.GetString( va( "loc_damage_area_%d", i ), "" ) );
		if ( info.area == LDA_INVALID ) {
			gameLocal.Warning( "Invalid locational damage area for joint %d", i );
			continue;
		}

//		gameLocal.Printf( "Adding jointPos: ( %.0f %.0f %.0f )\n", info.pos.x, info.pos.y, info.pos.z );
		owner->AddLocationalDamageInfo( info );
	}
}

/*
==============
anInventory::CheckPlayerClass
==============
*/
void anInventory::CheckPlayerClass( bool setWeapon ) {
	sdTeamInfo* team = owner->GetTeam();
	if ( !team ) {
		return;
	}

	const anDeclPlayerClass* newClass = playerClass.GetClass();

	anPlayerClassSetup oldCachedClassSetup = cachedPlayerClass;

	if ( cachedPlayerClass.GetClass() ) {
		if ( cachedPlayerClass.GetClass()->GetTeam() == team ) {
			newClass = cachedPlayerClass.GetClass();
		}

		SetCachedClass( nullptr );
	}

	if ( newClass ) {
		if ( newClass->GetTeam() != team ) {
			newClass = nullptr;
		}
	}

	if ( !newClass ) {
		newClass = team->GetDefaultClass();
	}

	bool sendInfo = false;

	if ( newClass != playerClass.GetClass() ) {
		sendInfo |= GiveClass( newClass, false );
	}

	if ( oldCachedClassSetup.GetClass() == playerClass.GetClass() ) {
		playerClass.SetOptions( oldCachedClassSetup.GetOptions() );
		SetupClassOptions( true, setWeapon );
		sendInfo = true;
	}

	if ( sendInfo ) {
		SendClassInfo( false );
	}
}

/*
==============
anInventory::IsIdealWeaponValid
==============
*/
bool anInventory::IsIdealWeaponValid( void ) const {
	return IsWeaponValid( idealWeapon );
}

/*
==============
anInventory::IsCurrentWeaponValid
==============
*/
bool anInventory::IsCurrentWeaponValid( void ) const {
	return IsWeaponValid( currentWeapon );
}

/*
==============
anInventory::GetCurrentItem
==============
*/
const anInventoryItem* anInventory::GetCurrentItem( void ) const {
	if ( currentWeapon < 0 || currentWeapon >= items.Num() ) {
		return nullptr;
	}

	if ( !CanEquip( currentWeapon, false ) ) {
		return nullptr;
	}

	return items[ currentWeapon ].GetItem();
}

/*
==============
anInventory::GetCurrentWeaponName
==============
*/
const char *anInventory::GetCurrentWeaponName( void ) {
	const anInventoryItem* item = GetCurrentItem();

	if ( !item ) {
		return nullptr;
	}

	return item->GetData().GetString( "anim_prefix" );
}

/*
==============
anInventory::GetCurrentWeaponClass
==============
*/
const char *anInventory::GetCurrentWeaponClass( void ) {
	const anInventoryItem* item = GetCurrentItem();

	if ( !item ) {
		return nullptr;
	}

	return item->GetData().GetString( "anim_prefix_class" );
}

/*
==============
anInventory::ClearAmmo
==============
*/
void anInventory::ClearAmmo( void ) {
	for ( int i = 0; i < ammo.Num(); i++ ) {
		ammo[ i ] = 0;
		ammoLimits[ i ] = 0;
	}
}

/*
==============
anInventory::GiveClass
==============
*/
bool anInventory::GiveClass( const anDeclPlayerClass* cls, bool sendInfo ) {
	if ( gameLocal.isClient ) {
		assert( false );
		return false;
	}

 	if ( cls == nullptr ) {
		gameLocal.Error( "anInventory::GiveClass: nullptr player class" );
		return false;
	}

	if ( cls->GetPackage() == nullptr ) {
		gameLocal.Error( "anInventory::GiveClass: nullptr package on player class '%s'", cls->GetName() );
		return false;
	}

	const sdTeamInfo* otherTeam = cls->GetTeam();
	if ( otherTeam != nullptr && otherTeam != owner->GetGameTeam() ) {
		return false;
	}

	if ( playerClass.GetClass() == cls ) {
		return false;
	}

	abEntity* proxy = owner->GetProxyEntity();
	if ( proxy != nullptr ) {
		proxy->GetUsableInterface()->OnExit( owner, true );
	}

	SetPlayerClass( cls );
	SelectBestWeapon( true );

	if ( sendInfo ) {
		SendClassInfo( false );
	}

	return true;
}

/*
==============
anInventory::SendClassInfo
==============
*/
void anInventory::SendClassInfo( bool cached ) {
	if ( !gameLocal.isServer ) {
		return;
	}

	anPlayerClassSetup& setup = cached ? cachedPlayerClass : playerClass;

	const anDeclPlayerClass* cls = setup.GetClass();

	sdEntityBroadcastEvent msg( owner, cached ? anBasePlayer::EVENT_SETCACHEDCLASS : anBasePlayer::EVENT_SETCLASS );
	msg.WriteBits( cls ? cls->Index() + 1 : 0, gameLocal.GetNumPlayerClassBits() );
	if ( cls ) {
		for ( int i = 0; i < cls->GetNumOptions(); i++ ) {
			const anDeclPlayerClass::optionList_t& option = cls->GetOption( i );
			msg.WriteBits( setup.GetOptions()[ i ], anMath::BitsForInteger( option.Num() ) );
		}
	}
	msg.Send( true, sdReliableMessageClientInfoAll() );
}

/*
==============
anInventory::SetupClassThread
==============
*/
void anInventory::SetupClassThread( void ) {
	ShutdownClassThread();
	if ( playerClass.GetClass() ) {
		const char *classThreadName = playerClass.GetClass()->GetClassThreadName();
		if ( *classThreadName ) {
			classThread = gameLocal.program->CreateThread();
			classThread->SetName( anStr( owner->GetName() ) + "_classThread" );
			classThread->CallFunction( owner->scriptObject, owner->scriptObject->GetFunction( classThreadName ) );
			classThread->ManualDelete();
			classThread->ManualControl();
			classThread->DelayedStart( 0 );
		}
	}
}

/*
==============
anInventory::ClearClass
==============
*/
void anInventory::ClearClass( void ) {
	SetPlayerClass( nullptr );
	SendClassInfo( false );
}

/*
==============
anInventory::LogClassTime
==============
*/
void anInventory::LogClassTime( void ) {
	const anDeclPlayerClass* pc = playerClass.GetClass();
	if ( pc != nullptr ) {
		const anDeclPlayerClass::stats_t& stats = pc->GetStats();
		if ( stats.timePlayed ) {
			int t = MS2SEC( gameLocal.time - timeClassChanged );
			stats.timePlayed->IncreaseValue( owner->entityNumber, t );
		}
	}
	timeClassChanged = gameLocal.time;
}

/*
==============
anInventory::SetPlayerClass
==============
*/
void anInventory::SetPlayerClass( const anDeclPlayerClass* cls ) {
	LogClassTime();
	SetCachedClass( nullptr );

	playerClass.SetClass( cls, true );

	SetupClassOptions( true, false );
	SetupClassThread();

	idScriptHelper h1;
	owner->CallFloatNonBlockingScriptEvent( owner->GetScriptObject()->GetFunction( "OnClassChanged" ), h1 );

	SetupModel();

	owner->UpdateRating();

	UpdatePlayerClassInfo( cls ); //mal: update this clients class info!
}

/*
==============
anInventory::SetClassOption
==============
*/
bool anInventory::SetClassOption( int optionIndex, int itemIndex, bool sendInfo ) {
	if ( !playerClass.SetOption( optionIndex, itemIndex ) ) {
		return false;
	}

	abEntity* proxy = owner->GetProxyEntity();
	if ( proxy != nullptr ) {
		proxy->GetUsableInterface()->OnExit( owner, true );
	}

	SetupClassOptions( true, true );

	if ( sendInfo ) {
		SendClassInfo( false );
	}

	return true;
}

/*
============
anInventory::SetCachedClass
============
*/
bool anInventory::SetCachedClass( const anDeclPlayerClass* pc, bool sendInfo ) {
	bool changed = cachedPlayerClass.SetClass( pc );

	if ( changed && sendInfo ) {
		SendClassInfo( true );
	}

	return changed;
}

/*
============
anInventory::SetCachedClassOption
============
*/
bool anInventory::SetCachedClassOption( int optionIndex, int itemIndex, bool sendInfo ) {
	if ( optionIndex < 0 || optionIndex >= cachedPlayerClass.GetOptions().Num() ) {
		return false;
	}

	bool changed = cachedPlayerClass.SetOption( optionIndex, itemIndex );

	if ( changed && sendInfo ) {
		SendClassInfo( true );
	}

	return changed;
}

/*
==============
anInventory::AddItems
==============
*/
bool anInventory::AddItems( const sdDeclItemPackage* package, bool enabled ) {
	const sdDeclItemPackageNode& node = package->GetItemRoot();
	return AddItemNode( node, enabled, false );
}

/*
==============
anInventory::CheckItems
==============
*/
bool anInventory::CheckItems( const sdDeclItemPackage* package ) {
	const sdDeclItemPackageNode& node = package->GetItemRoot();
	return AddItemNode( node, true, true );
}

/*
==============
anInventory::AddItemNode
==============
*/
bool anInventory::AddItemNode( const sdDeclItemPackageNode& node, bool enabled, bool testOnly ) {
	bool added = false;

	if ( !node.GetRequirements().Check( owner ) ) {
		enabled = false;
	}

	for ( int i = 0; i < node.GetItems().Num(); i++ ) {
		const anInventoryItem* item = node.GetItems()[ i ];
		if ( !testOnly ) {
			items.AddItem( item, enabled );
		}
		added |= enabled;
	}

	for ( int i = 0; i < node.GetNodes().Num(); i++ ) {
		added |= AddItemNode( *node.GetNodes()[ i ], enabled, testOnly );
	}

	return added;
}

/*
==============
anInventory::SetupClassOptions
==============
*/
void anInventory::SetupClassOptions( bool clearAmmo, bool setWeapon, bool allowCurrentWeapon ) {
	bool disguise = owner->IsDisguised();
	if ( disguise ) {
		items.Clear();

		const anDeclPlayerClass* cls = owner->GetDisguiseClass();
		if ( cls == nullptr ) {
			return;
		}

		AddItems( cls->GetDisguisePackage() );
	} else { 

		if ( clearAmmo ) {
			ClearAmmo();
		}

		items.Clear();

		const anDeclPlayerClass* cls = playerClass.GetClass();
		if ( cls == nullptr ) {
			return;
		}

		if ( clearAmmo ) {
			for ( int i = 0; i < ammoLimits.Num(); i++ ) {
				ammoLimits[ i ] = cls->GetAmmoLimit( i );
			}
		}

		const sdDeclItemPackage* package = cls->GetPackage();
		AddItems( package );
		if ( clearAmmo && !disguise ) {
			GiveConsumables( package );
		}

		for ( int i = 0; i < playerClass.GetOptions().Num(); i++ ) {
			const anDeclPlayerClass::optionList_t& list = cls->GetOption( i );

			int index = playerClass.GetOptions()[ i ];
			if ( index < 0 || index >= list.Num() ) {
				index = 0;
			}

			if ( !CheckItems( list[ index ] ) ) {
				index = 0;
			}

			for ( int j = 0; j < list.Num(); j++ ) {
				AddItems( list[ j ], j == index );
			}

			if ( clearAmmo ) {
				GiveConsumables( list[ index ] );
			}
		}
	}

	SortClips();
	if ( !gameLocal.isClient && setWeapon ) {
		SelectBestWeapon( allowCurrentWeapon );
	}
}

/*
==============
anInventory::HasAbility
==============
*/
bool anInventory::HasAbility( qhandle_t handle ) const {
	const anDeclPlayerClass* cls = playerClass.GetClass();
	return cls && cls->HasAbility( handle );
}

/*
==============
anInventory::SortClips
==============
*/
void anInventory::SortClips( void ) {
	for ( int i = 0; i < items.Num(); i++ ) {
		const anInventoryItem* item = GetItem( i );
		for( int clip = 0; clip < item->GetClips().Num(); clip++ ) {
			const itemClip_t& clipInfo = item->GetClips()[ clip ];
			if ( clipInfo.maxAmmo > 0 && clipInfo.ammoPerShot > 0 ) {
				int max = Min( clipInfo.maxAmmo, GetAmmo( clipInfo.ammoType ) );
				SetClip( i, clip, max );
			}
		}
	}
}

/*
==============
anInventory::GiveConsumables
==============
*/
bool anInventory::GiveConsumablesNode( const sdDeclItemPackageNode& node ) {
	if ( !node.GetRequirements().Check( owner ) ) {
		return false;
	}

	bool given = false;

	anList<const sdConsumable *> consumables = node.GetConsumables();
	for ( int i = 0; i < consumables.Num(); i++ ) {
		if ( !consumables[ i ]->Give( owner ) ) {
			continue;
		}
		given = true;
	}

	for ( int i = 0; i < node.GetNodes().Num(); i++ ) {
		given |= GiveConsumablesNode( *node.GetNodes()[ i ] );
	}

	return given;
}

/*
==============
anInventory::GiveConsumables
==============
*/
bool anInventory::GiveConsumables( const sdDeclItemPackage* package ) {
	return GiveConsumablesNode( package->GetItemRoot() );
}

/*
==============
anInventory::GetWeaponTitle
==============
*/
const sdDeclLocStr* anInventory::GetWeaponTitle( void ) const {
	if ( IsSwitchActive() ) {
		return items[ switchingWeapon ].item->GetItemName();
	}

	if ( !IsCurrentWeaponValid() ) {
		return nullptr;
	}

	return items[ currentWeapon ].item->GetItemName();
}

/*
==============
anInventory::GetWeaponName
==============
*/
const char *anInventory::GetWeaponName( void ) const {
	if ( IsSwitchActive() ) {
		return items[ switchingWeapon ].item->GetName();
	}

	if ( !IsCurrentWeaponValid() ) {
		return "";
	}

	return items[ currentWeapon ].item->GetName();
}

/*
============
anInventory::CheckWeaponHasAmmo
============
*/
bool anInventory::CheckWeaponHasAmmo( const anInventoryItem* item ) const {
	const anList< itemClip_t >& itemClips = item->GetClips();

	if ( !itemClips.Num() ) {
		return true;
	}

	for ( int i = 0; i < itemClips.Num(); i++ ) {
		if ( GetAmmo( itemClips[ i ].ammoType ) >= itemClips[ i ].ammoPerShot ) {
			return true;
		}
	}
	return false;
}

/*
============
anInventory::GetAmmo
============
*/
int anInventory::GetAmmo( int index ) const {
	return ammo[ index ];
}

/*
==============
anInventory::ShutdownClassThread
==============
*/
void anInventory::ShutdownClassThread( void ) {
	if ( classThread != nullptr ) {
		gameLocal.program->FreeThread( classThread );
		classThread = nullptr;
	}
}

/*
============
anInventory::CancelWeaponSwitch
============
*/
void anInventory::CancelWeaponSwitch( void ) {
	switchingWeapon = -1;
}

/*
============
anInventory::AcceptWeaponSwitch
============
*/
void anInventory::AcceptWeaponSwitch( void ) {
	if ( CanEquip( switchingWeapon, true ) ) {
		SetIdealWeapon( switchingWeapon );
		switchingWeapon = -1;
	}
}

/*
============
anInventory::UpdateCurrentWeapon
============
*/
void anInventory::UpdateCurrentWeapon( void ) {
	if ( currentWeapon != idealWeapon ) {
		if ( currentWeapon >= 0 && currentWeapon < items.Num() ) {
			if ( items.ShowItem( items[ currentWeapon ] ) ) {
				items[ currentWeapon ].SetVisible( true );
			}
		}

		currentWeapon = idealWeapon;

		if ( currentWeapon >= 0 && currentWeapon < items.Num() ) {
			items[ currentWeapon ].SetVisible( false );
			items.HideItem( items[ currentWeapon ] );
		}
	}

	weaponChanged = false;
}

/*
============
anInventory::ApplyPlayerState
============
*/
void anInventory::ApplyPlayerState( const anInventoryPlayerStateData& newState ) {
	NET_GET_NEW( anInventoryPlayerStateData );

	for ( int i = 0; i < newData.ammo.Num(); i++ ) {
		SetAmmo( i, newData.ammo[ i ] );
	}

	items.ApplyPlayerState( newData );
}

/*
============
anInventory::ReadPlayerState
============
*/
void anInventory::ReadPlayerState( const anInventoryPlayerStateData& baseState, anInventoryPlayerStateData& newState, const anBitMsg& msg ) const {
	//NET_GET_STATES( anInventoryPlayerStateData );

	newData.ammo.SetNum( gameLocal.declAmmoTypeType.Num() );
	for ( int i = 0; i < newData.ammo.Num(); i++ ) {
		newData.ammo[ i ] = msg.ReadDeltaShort( baseData.ammo[ i ] );
	}

	items.ReadPlayerState( baseState, newData, msg );
}

/*
============
anInventory::WritePlayerState
============
*/
void anInventory::WritePlayerState( const anInventoryPlayerStateData& baseState, anInventoryPlayerStateData& newState, anBitMsg& msg ) const {
	//NET_GET_STATES( anInventoryPlayerStateData );

	newData.ammo.SetNum( gameLocal.declAmmoTypeType.Num() );
	for ( int i = 0; i < newData.ammo.Num(); i++ ) {
		newData.ammo[ i ] = GetAmmo( i );
		msg.WriteDeltaShort( baseData.ammo[ i ], newData.ammo[ i ] );
	}

	items.WritePlayerState( baseState, newState, msg );
}

/*
============
anInventory::CheckPlayerStateChanges
============
*/
bool anInventory::CheckPlayerStateChanges( const anInventoryPlayerStateData& baseState ) const {
	//NET_GET_BASE( anInventoryPlayerStateData );

	for ( int i = 0; i < baseData.ammo.Num(); i++ ) {
		if ( baseData.ammo[ i ] != GetAmmo( i ) ) {
			return true;
		}
	}

	return items.CheckPlayerStateChanges( baseState );
}


/*
============
anInventory::HideCurrentItem
============
*/
void anInventory::HideCurrentItem( bool hide ) {
	if ( currentWeapon != -1 ) {
		if ( hide ) {
			items[ currentWeapon ].SetVisible( false );
			items.HideItem( items[ currentWeapon ] );
		} else {
			if ( items.ShowItem( items[ currentWeapon ] ) ) {
				items[ currentWeapon ].SetVisible( true );
			}
		}
	}
}

/*
============
anInventory::SetClass
============
*/
bool anInventory::SetClass( const anBitMsg& msg, bool cached ) {
	anPlayerClassSetup& setup = cached ? cachedPlayerClass : playerClass;

	int playerClassIndex = msg.ReadBits( gameLocal.GetNumPlayerClassBits() ) - 1;
	const anDeclPlayerClass* pc = nullptr;
	if ( playerClassIndex != -1 ) {
		pc = gameLocal.declPlayerClassType[ playerClassIndex ];
	}

	if ( setup.GetClass() != pc ) {
		if ( cached ) {
			SetCachedClass( pc );
		} else {
			SetPlayerClass( pc );
		}
	}

	if ( pc ) {
		bool changed = false;
		for ( int i = 0; i < pc->GetNumOptions(); i++ ) {
			int itemIndex = msg.ReadBits( anMath::BitsForInteger( pc->GetOption( i ).Num() ) );
			if ( setup.GetOptions()[ i ] != itemIndex ) {
				setup.SetOption( i, itemIndex );
				changed = true;
			}
		}
		if ( changed && !cached ) {
			SetupClassOptions( true, false );
		}
	}

	return true;
}

/*
============
anInventory::UpdateForDisguise
============
*/
void anInventory::UpdateForDisguise( void ) {
	SetupClassOptions( false, true, false );
	SetupModel();
}

/*
============
anInventory::OnHide
============
*/
void anInventory::OnHide( void ) {
	items.OnHide();
}

/*
============
anInventory::OnShow
============
*/
void anInventory::OnShow( void ) {
	items.OnShow();
}

/*
============
anInventory::LogSuicide
============
*/
void anInventory::LogSuicide( void ) {
	const anDeclPlayerClass* pc = playerClass.GetClass();
	if ( !pc ) {
		return;
	}

	const anDeclPlayerClass::stats_t &stats = pc->GetStats();
	if ( stats.suicides ) {
		stats.suicides->IncreaseValue( owner->entityNumber, 1 );
	}
}

/*
============
anInventory::LogDeath
============
*/
void anInventory::LogDeath( void ) {
	const anDeclPlayerClass *pc = playerClass.GetClass();
	if ( !pc ) {
		return;
	}

	const anDeclPlayerClass::stats_t& stats = pc->GetStats();
	if ( stats.deaths != nullptr ) {
		stats.deaths->IncreaseValue( owner->entityNumber, 1 );
	}
}

/*
============
anInventory::LogRevive
============
*/
void anInventory::LogRevive( void ) {
	const anDeclPlayerClass* pc = playerClass.GetClass();
	if ( !pc ) {
		return;
	}

	const anDeclPlayerClass::stats_t& stats = pc->GetStats();
	if ( stats.revived ) {
		stats.revived->IncreaseValue( owner->entityNumber, 1 );
	}
}

/*
============
anInventory::LogTapOut
============
*/
void anInventory::LogTapOut( void ) {
	const anDeclPlayerClass* pc = playerClass.GetClass();
	if ( !pc ) {
		return;
	}

	const anDeclPlayerClass::stats_t& stats = pc->GetStats();
	if ( stats.tapouts ) {
		stats.tapouts->IncreaseValue( owner->entityNumber, 1 );
	}
}

/*
============
anInventory::LogRespawn
============
*/
void anInventory::LogRespawn( void ) {
	const anDeclPlayerClass* pc = playerClass.GetClass();
	if ( !pc ) {
		return;
	}

	const anDeclPlayerClass::stats_t& stats = pc->GetStats();
	if ( stats.respawns != nullptr ) {
		stats.respawns->IncreaseValue( owner->entityNumber, 1 );
	}
}

/*
============
anInventory::UpdatePlayerClassInfo

update the players class info game side for the bots.
============
*/
void anInventory::UpdatePlayerClassInfo( const anDeclPlayerClass *pc ) {
 	if ( !pc ) {
		return;
	}
	
	botThreadData.GetGameWorldState()->clientInfo[owner->entityNumber].classType = pc->GetPlayerClassNum();
}

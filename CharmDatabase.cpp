#include "stdafx.h"
#include "CharmDatabase.h"
#include "Solution.h"
#include "Armor.h"
#include "Skill.h"
#include <fstream>
#include <cmath>

using namespace System;

/*static const unsigned TotalTables = 19;
static const unsigned AllTable = TotalTables - 1;

template< typename T >
void MaxOut( T% curr, const int num )
{
	curr = Math::Max( curr, (T)num );
}

template< typename T >
void MinOut( T% curr, const int num )
{
	curr = Math::Min( curr, (T)num );
}*/

unsigned CalcMaxCharmType( Query^ query )
{
	if( query->hr >= 5 )
	{
		return ( query->hr >= 8 ) ? 3 : 2;
	}
	else
	{
		return ( query->hr >= 4 ) ? 1 : 0;
	}
}

#pragma region Custom shit

#define CUSTOM_GEAR_TXT L"Data/mygear.txt"

void ExcavatedGear::SaveCustom()
{
	IO::StreamWriter fout( CUSTOM_GEAR_TXT );
	for each( Weapon^ wep in weapons )
	{
		fout.Write( "5," );
		fout.Write( Convert::ToString( (int)wep->type ) + "," );
		fout.Write( Convert::ToString( wep->ability_amount ) + "," );
		fout.Write( Convert::ToString( wep->ability->name ) );
		fout.Write( Convert::ToString( wep->defence ) );
		fout.WriteLine();
	}
	for( int i = 0; i < armor->Length; ++i )
	{
		for( int j = 0; j < armor[ i ]->Count; ++j )
		{
			Armor^ arm = armor[ i ][ j ];
			fout.Write( Convert::ToString( i ) + L"," );
			fout.Write( Convert::ToString( (int)arm->type ) + L"," );
			fout.Write( Convert::ToString( arm->abilities[ 0 ]->amount ) + "," );
			fout.Write( Convert::ToString( arm->abilities[ 0 ]->ability->name ) + L"," );

			fout.Write( Convert::ToString( arm->defence ) + L"," );
			fout.Write( Convert::ToString( arm->fire_res ) + L"," );
			fout.Write( Convert::ToString( arm->water_res ) + L"," );
			fout.Write( Convert::ToString( arm->ice_res ) + L"," );
			fout.Write( Convert::ToString( arm->thunder_res ) + L"," );
			fout.Write( Convert::ToString( arm->dragon_res ) + L"," );
			fout.WriteLine();
		}
	}
}

void ExcavatedGear::LoadCustom()
{
	weapons.Clear();
	armor = gcnew array< List_t< Armor^ >^ >( (int)Armor::ArmorType::NumArmorTypes );
	for( int i = 0; i < armor->Length; ++i )
		armor[ i ] = gcnew List_t< Armor^ >();

	if( !IO::File::Exists( CUSTOM_GEAR_TXT ) )
	{
		return;
	}

	IO::StreamReader fin( CUSTOM_GEAR_TXT );
	String^ temp;
	while( !fin.EndOfStream )
	{
		temp = fin.ReadLine();
		if( temp == L"" || temp[ 0 ] == L'#' )
			continue;

		List_t< String^ > split;
		Utility::SplitString( %split, temp, L',' );
		
		const int type = Convert::ToUInt32( split[ 0 ] );
		if( type == (int)Armor::ArmorType::NumArmorTypes )
		{
			Weapon^ wep = gcnew Weapon();
			wep->type = (HunterType)Convert::ToUInt32( split[ 1 ] );
			wep->ability_amount = Convert::ToUInt32( split[ 2 ] );
			wep->ability = Ability::FindAbility( split[ 3 ] );
			wep->defence = split.Count > 4 ? Convert::ToUInt32( split[ 4 ] ) : 0;
			wep->index = weapons.Count;
			if( wep->ability )
				weapons.Add( wep );
		}
		else
		{
			Armor^ arm = gcnew Armor();
			arm->type = (HunterType)Convert::ToUInt32( split[ 1 ] );
			int amount = Convert::ToUInt32( split[ 2 ] );
			Ability^ ability = Ability::FindAbility( split[ 3 ] );
			if( ability )
			{
				arm->abilities.Add( gcnew AbilityPair( ability, amount ) );
				arm->defence = Convert::ToUInt32( split[ 4 ] );
				arm->fire_res = Convert::ToInt32( split[ 5 ] );
				arm->water_res = Convert::ToInt32( split[ 6 ] );
				arm->ice_res = Convert::ToInt32( split[ 7 ] );
				arm->thunder_res = Convert::ToInt32( split[ 8 ] );
				arm->dragon_res = Convert::ToInt32( split[ 9 ] );
				arm->SetExName( type );
				arm->index = armor[ type ]->Count;
				arm->gender = Gender::BOTH_GENDERS;
				armor[ type ]->Add( arm );
			}
		}
	}
}

#define CUSTOM_CHARM_TXT L"Data/mycharms.txt"

void CharmDatabase::SaveCustom()
{
	//slots,skill1,num,skill2,num
	IO::StreamWriter fout( CUSTOM_CHARM_TXT );
	fout.WriteLine( L"#Format: Slots,Skill1,Points1,Skill2,Points2" );
	for each( Charm^ ch in mycharms )
	{
		fout.Write( Convert::ToString( ch->num_slots ) );
		for( int i = 0; i < 2; ++i )
		{
			if( i < ch->abilities.Count )
				fout.Write( L"," + ch->abilities[ i ]->ability->name + L"," + Convert::ToString( ch->abilities[ i ]->amount ) );
			else fout.Write( L",," );
		}
		fout.WriteLine();
	}
}

String^ FixTypos( String^ input )
{
	if( input == L"ElementAtk" )
		return L"Elemental";
	else if( input == L"EdgeMaster" )
		return L"Edgemaster";
	else if( input == L"Consitution" )
		return L"Constitution";
	else return input;
}

void CharmDatabase::LoadCustom()
{
	mycharms.Clear();
	if( !IO::File::Exists( CUSTOM_CHARM_TXT ) )
	{
		return;
	}

	IO::StreamReader fin( CUSTOM_CHARM_TXT );
	String^ temp;
	bool cheats = false;
	while( !fin.EndOfStream )
	{
		temp = fin.ReadLine();
		if( temp == L"" || temp[ 0 ] == L'#' ) continue;
		List_t< String^ > split;
		Utility::SplitString( %split, temp, L',' );
		if( split.Count != 5 )
		{
			Windows::Forms::MessageBox::Show( nullptr, L"Incorrect number of commas", temp );
			continue;
		}
		//slots,skill1,num,skill2,num
		Charm^ charm = gcnew Charm();
		charm->num_slots = Convert::ToUInt32( split[ 0 ] );
		
		try
		{
			if( split[ 1 ] != L"" )
			{
				split[ 1 ] = FixTypos( split[ 1 ] );
				Ability^ ab = Ability::FindAbility( split[ 1 ] );
				if( !ab )
				{
					Windows::Forms::MessageBox::Show( nullptr, L"\"" + split[ 1 ] + L"\": No such skill", temp );
					continue;
				}
				charm->abilities.Add( gcnew AbilityPair( ab, Convert::ToInt32( split[ 2 ] ) ) );
			}
			if( split[ 3 ] != L"" )
			{
				split[ 3 ] = FixTypos( split[ 3 ] );
				Ability^ ab = Ability::FindAbility( split[ 3 ] );
				if( !ab )
				{
					Windows::Forms::MessageBox::Show( nullptr, L"\"" + split[ 3 ] + L"\": No such skill", temp );
					continue;
				}
				charm->abilities.Add( gcnew AbilityPair( ab, Convert::ToInt32( split[ 4 ] ) ) );
			}
		}
		catch( Exception^ )
		{
			Windows::Forms::MessageBox::Show( nullptr, L"Could not read skill points", temp );
			continue;
		}
		
		if( CharmDatabase::CharmIsLegal( charm ) )
		{
			charm->custom = true;
			mycharms.Add( charm );
		}
		else
		{
			cheats = true;
		}
	}
	if( cheats )
		System::Windows::Forms::MessageBox::Show( StaticString( Cheater ) );
}

#pragma endregion

#pragma region Charm Generation
ref struct StaticData
{
	static array< array< unsigned char >^ >^ skill1_table;
	static array< array<   signed char >^ >^ skill2_table; // [type][skill]
	static array< array< unsigned char >^ >^ slot_table;

	static array< unsigned char >^ skill2_chance_table =
	{
		100,
		35,
		25,
		20
	};

};

int rnd( const int n )
{
	Assert( n < 65536 && n >= 0, L"Bad RND" );
	if( n == 0 ) return 176;

	return ( n * 176 ) % 65363;
}

int reverse_rnd( const int n )
{
	return ( n * 7799 ) % 65363;
}

unsigned GetNumSlots( const unsigned charm_type, const int slot_table, const unsigned roll )
{
	Assert( (int)charm_type < StaticData::slot_table->Length, L"Bad charm type" );

	const unsigned table_index = Math::Min( slot_table, StaticData::slot_table[ charm_type ]->Length / 3 ) - 1;
	const unsigned for2 = StaticData::slot_table[ charm_type ][ table_index * 3 + 1 ];
	const unsigned for3 = StaticData::slot_table[ charm_type ][ table_index * 3 + 2 ];
	const unsigned for1 = StaticData::slot_table[ charm_type ][ table_index * 3 + 0 ];
	if( roll >= for2 )
	{
		return ( roll >= for3 ) ? 3 : 2;
	}
	else
	{
		return ( roll >= for1 ) ? 1 : 0;
	}
}

bool TryTwoSkillCharm( const unsigned charm_type, int n, int m, array< List_t< unsigned >^ >^ charms )
{
	array< unsigned char >^ skill1_table = StaticData::skill1_table[ charm_type ];
	array< signed char >^	skill2_table = StaticData::skill2_table[ charm_type ];
	const unsigned num_skills1 = skill1_table->Length / 3;
	const unsigned num_skills2 = skill2_table == nullptr ? 0 : skill2_table->Length / 3;

	//skill 1
	const int skill1_index = n % num_skills1;
	const int skill1_name = skill1_table[ skill1_index * 3 ];
	const int skill1_min = skill1_table[ skill1_index * 3 + 1 ];
	const int skill1_max = skill1_table[ skill1_index * 3 + 2 ];

	//skill 1 point
	n = rnd( n );
	const int point1 = skill1_min + (n^m) % ( skill1_max - skill1_min + 1 );

	//has skill 2?
	int skill2_index = -1, point2 = 0, skill2_min = 0, skill2_max = 0, skill2_name = 0;
	n = rnd( n );
	m = rnd( m );
	if( (n^m) % 100 >= StaticData::skill2_chance_table[ charm_type ] )
	{
		//skill 2
		m = rnd( m );
		skill2_index = m % num_skills2;
		skill2_name = skill2_table[ skill2_index * 3 ];
		skill2_min = skill2_table[ skill2_index * 3 + 1 ];
		skill2_max = skill2_table[ skill2_index * 3 + 2 ];

		//skill 2 point
		if( skill2_min > 0 ) //positive number
		{
			n = rnd( n );
			m = rnd( m );
			point2 = ( (n^m) % ( skill2_max - skill2_min + 1 ) ) + skill2_min;
		}
		else //check for negative
		{
			n = rnd( n );
			if( n % 2 == 1 ) //positive
			{
				n = rnd( n );
				m = rnd( m );
				point2 = (n^m) % skill2_max + 1;
			}
			else //negative
			{
				n = rnd( n );
				m = rnd( m );
				point2 = skill2_min + (n^m) % ( -skill2_min );
			}
		}
		if( point2 < 0 )
			return false;

		if( skill1_name == skill2_name )
		{
			skill2_min = skill2_max = point2 = 0;
			skill2_name = skill2_index = Ability::static_abilities.Count;
		}
	}

	const int slot_table = (int)Math::Floor( point1*10.0 / skill1_max + ( (point2 > 0) ? point2*10.0 / skill2_max : 0 ) );

	//slots
	n = rnd( n );
	int slot_roll = 0;
	if( n & 128 )
	{
		n = rnd( n );
		slot_roll = n % 100;
	}
	else
	{
		m = rnd( m );
		slot_roll = m % 100;
	}

	const int num_slots = GetNumSlots( charm_type, slot_table, slot_roll );

	List_t< unsigned >^ list = charms[ num_slots ];
	for( int i = 0; i < list->Count; ++i )
	{
		const unsigned hash = list[i];
		const int p1 = hash >> 16;
		const int p2 = hash & 0xFFFF;
		if( p1 >= point1 && p2 >= point2 )
			return false;
	}
	list->Add( point1 << 16 | point2 );

	return num_slots == Math::Min( 3u, charm_type + 1 ) && slot_table == 20;
}

String^ CanGenerateCharm1( const unsigned charm_type, int n, int m, Charm^ charm )
{
	Assert( charm->abilities.Count == 1, "Too many abilities for charm" );
	Assert( (int)charm_type < StaticData::skill1_table->Length && (int)charm_type < StaticData::skill2_table->Length, "Charm type is invalid" );
	array< unsigned char >^ skill1_table = StaticData::skill1_table[ charm_type ];
	array< signed char >^	skill2_table = StaticData::skill2_table[ charm_type ];
	const unsigned num_skills1 = skill1_table->Length / 3;
	const unsigned num_skills2 = skill2_table->Length / 3;

	//skill 1
	const int skill1_index = n % num_skills1;
	const int skill1_name = skill1_table[ skill1_index * 3 ];
	const int skill1_min = skill1_table[ skill1_index * 3 + 1 ];
	const int skill1_max = skill1_table[ skill1_index * 3 + 2 ];

	//skill 1 point
	n = rnd( n );
	const int point1 = skill1_min + (n^m) % ( skill1_max - skill1_min + 1 );

	if( skill1_name == charm->abilities[0]->ability->static_index && point1 < charm->abilities[0]->amount )
		return nullptr;

	//has skill 2?
	int skill2_index = -1, point2 = 0, skill2_min = 0, skill2_max = 0, skill2_name = 0;
	n = rnd( n );
	m = rnd( m );
	if( (n^m) % 100 >= StaticData::skill2_chance_table[ charm_type ] )
	{
		//skill 2
		m = rnd( m );
		skill2_index = m % num_skills2;
		skill2_name = skill2_table[ skill2_index * 3 ];
		skill2_min = skill2_table[ skill2_index * 3 + 1 ];
		skill2_max = skill2_table[ skill2_index * 3 + 2 ];

		//skill 2 point
		if( skill2_min > 0 ) //positive number
		{
			n = rnd( n );
			m = rnd( m );
			point2 = ( (n^m) % ( skill2_max - skill2_min + 1 ) ) + skill2_min;
		}
		else //check for negative
		{
			n = rnd( n );
			if( n % 2 == 1 ) //positive
			{
				n = rnd( n );
				m = rnd( m );
				point2 = (n^m) % skill2_max + 1;
			}
			else //negative
			{
				n = rnd( n );
				m = rnd( m );
				point2 = skill2_min + (n^m) % ( -skill2_min );
			}
		}

		if( skill1_name == skill2_name )
		{
			skill2_name = point2 = 0;
			skill2_index = -1;
		}

		if( skill1_name != charm->abilities[0]->ability->static_index && skill2_name != charm->abilities[0]->ability->static_index ||
			point2 < charm->abilities[0]->amount )
			return nullptr;
	}

	const int slot_table = (int)Math::Floor( point1*10.0 / skill1_max + ( (point2 > 0) ? point2*10.0 / skill2_max : 0 ) );

	//slots
	n = rnd( n );
	int slot_roll = 0;
	if( n & 128 )
	{
		n = rnd( n );
		slot_roll = n % 100;
	}
	else
	{
		m = rnd( m );
		slot_roll = m % 100;
	}

	const unsigned num_slots = GetNumSlots( charm_type, slot_table, slot_roll );

	if( num_slots < charm->num_slots )
		return nullptr;

	Charm c( num_slots );
	c.abilities.Add( gcnew AbilityPair( Ability::static_abilities[ skill1_name ], point1 ) );
	if( point2 )
		c.abilities.Add( gcnew AbilityPair( Ability::static_abilities[ skill2_name ], point2 ) );

	return c.GetName();
}

String^ CanGenerateCharm2( const unsigned charm_type, int n, int m, Charm^ charm )
{
	Assert( charm->abilities.Count == 2, "Too few abilities for charm" );
	Assert( (int)charm_type < StaticData::skill1_table->Length && (int)charm_type < StaticData::skill2_table->Length, "Charm type is invalid" );
	array< unsigned char >^ skill1_table = StaticData::skill1_table[ charm_type ];
	array< signed char >^	skill2_table = StaticData::skill2_table[ charm_type ];
	const unsigned num_skills1 = skill1_table->Length / 3;
	const unsigned num_skills2 = skill2_table->Length / 3;

	//skill 1
	const int skill1_index = n % num_skills1;
	const int skill1_name = skill1_table[ skill1_index * 3 ];
	const int skill1_min = skill1_table[ skill1_index * 3 + 1 ];
	const int skill1_max = skill1_table[ skill1_index * 3 + 2 ];

	//skill 1 point
	n = rnd( n );
	const int point1 = skill1_min + (n^m) % ( skill1_max - skill1_min + 1 );

	if( point1 < charm->abilities[0]->amount )
		return nullptr;

	//has skill 2?
	int skill2_index = -1, point2 = 0, skill2_min = 0, skill2_max = 0, skill2_name = 0;
	n = rnd( n );
	m = rnd( m );
	if( (n^m) % 100 >= StaticData::skill2_chance_table[ charm_type ] )
	{
		//skill 2
		m = rnd( m );
		skill2_index = m % num_skills2;
		skill2_name = skill2_table[ skill2_index * 3 ];
		skill2_min = skill2_table[ skill2_index * 3 + 1 ];
		skill2_max = skill2_table[ skill2_index * 3 + 2 ];

		//skill 2 point
		if( skill2_min > 0 ) //positive number
		{
			n = rnd( n );
			m = rnd( m );
			point2 = ( (n^m) % ( skill2_max - skill2_min + 1 ) ) + skill2_min;
		}
		else //check for negative
		{
			n = rnd( n );
			if( n % 2 == 1 ) //positive
			{
				n = rnd( n );
				m = rnd( m );
				point2 = (n^m) % skill2_max + 1;
			}
			else //negative
			{
				n = rnd( n );
				m = rnd( m );
				point2 = skill2_min + (n^m) % ( -skill2_min );
			}
		}

		if( skill1_name == skill2_name || point2 < charm->abilities[1]->amount )
			return nullptr;
	}
	else return nullptr;

	const int slot_table = (int)Math::Floor( point1*10.0 / skill1_max + ( (point2 > 0) ? point2*10.0 / skill2_max : 0 ) );

	//slots
	n = rnd( n );
	int slot_roll = 0;
	if( n & 128 )
	{
		n = rnd( n );
		slot_roll = n % 100;
	}
	else
	{
		m = rnd( m );
		slot_roll = m % 100;
	}

	const unsigned num_slots = GetNumSlots( charm_type, slot_table, slot_roll );

	if( num_slots < charm->num_slots )
		return nullptr;

	Charm c( num_slots );
	c.abilities.Add( gcnew AbilityPair( Ability::static_abilities[ skill1_name ], point1 ) );
	c.abilities.Add( gcnew AbilityPair( Ability::static_abilities[ skill2_name ], point2 ) );

	return c.GetName();
}

bool CanGenerate2SkillCharm( const unsigned charm_type, int n, int m, Charm^ charm )
{
	Assert( charm->abilities.Count == 2, "Charm has too few abilities" );
	Assert( charm_type > 0 && (int)charm_type < StaticData::skill1_table->Length && (int)charm_type < StaticData::skill2_table->Length, "Charm type is invalid" );
	array< unsigned char >^ skill1_table = StaticData::skill1_table[ charm_type ];
	array< signed char >^	skill2_table = StaticData::skill2_table[ charm_type ];
	const unsigned num_skills1 = skill1_table->Length / 3;
	const unsigned num_skills2 = skill2_table->Length / 3;

	//skill 1
	//n = rnd( n );
	const int skill1_index = n % num_skills1;
	const int skill1_name = skill1_table[ skill1_index * 3 ];
	Assert( skill1_name == charm->abilities[ 0 ]->ability->static_index, "Skill1 failed" );
	const int skill1_min = skill1_table[ skill1_index * 3 + 1 ];
	const int skill1_max = skill1_table[ skill1_index * 3 + 2 ];

	//skill 1 point
	n = rnd( n );
	//m = rnd( m );
	const int point1 = skill1_min + (n^m) % ( skill1_max - skill1_min + 1 );

	if( point1 < charm->abilities[0]->amount )
		return false;

	//has skill 2?
	int skill2_index = -1, point2 = 0, skill2_min = 0, skill2_max = 0, skill2_name = 0;
	n = rnd( n );
	m = rnd( m );
	if( (n^m) % 100 >= StaticData::skill2_chance_table[ charm_type ] )
	{
		//skill 2
		m = rnd( m );
		skill2_index = m % num_skills2;
		skill2_name = skill2_table[ skill2_index * 3 ];
		Assert( skill2_name == charm->abilities[1]->ability->static_index, "Skill2 failed" );
		skill2_min = skill2_table[ skill2_index * 3 + 1 ];
		skill2_max = skill2_table[ skill2_index * 3 + 2 ];

		//skill 2 point
		if( skill2_min > 0 ) //positive number
		{
			n = rnd( n );
			m = rnd( m );
			point2 = ( (n^m) % ( skill2_max - skill2_min + 1 ) ) + skill2_min;
		}
		else //check for negative
		{
			n = rnd( n );
			if( n % 2 == 1 ) //positive
			{
				n = rnd( n );
				m = rnd( m );
				point2 = (n^m) % skill2_max + 1;
			}
			else //negative
			{
				n = rnd( n );
				m = rnd( m );
				point2 = skill2_min + (n^m) % ( -skill2_min );
			}
		}

		if( skill1_name == skill2_name || point2 < charm->abilities[1]->amount )
		{
			return false;
		}
	}
	else return false;

	const int slot_table = (int)Math::Floor( point1*10.0 / skill1_max + ( (point2 > 0) ? point2*10.0 / skill2_max : 0 ) );

	//slots
	n = rnd( n );
	int slot_roll = 0;
	if( n & 128 )
	{
		n = rnd( n );
		slot_roll = n % 100;
	}
	else
	{
		m = rnd( m );
		slot_roll = m % 100;
	}

	const unsigned num_slots = GetNumSlots( charm_type, slot_table, slot_roll );

	return num_slots >= charm->num_slots;
}

bool GenerateCharm3( const unsigned charm_type, const unsigned table, int n, int m, Charm^ charm )
{
	//check charm_type < StaticData::skill1_table->Length
	array< unsigned char >^ skill1_table = StaticData::skill1_table[ charm_type ];
	const unsigned num_skills1 = skill1_table->Length / 3;
	//check charm_type < StaticData::skill2_table->Length
	array< signed char >^ skill2_table = StaticData::skill2_table[ charm_type ];
	const unsigned num_skills2 = skill2_table == nullptr ? 0 : skill2_table->Length / 3;

	//skill 1
	//n = rnd( n );
	const int skill1_index = n % num_skills1;
	const int skill1_name = skill1_table[ skill1_index * 3 ];
	const int skill1_min = skill1_table[ skill1_index * 3 + 1 ];
	const int skill1_max = skill1_table[ skill1_index * 3 + 2 ];

	//skill 1 point
	n = rnd( n );
	//m = rnd( m );
	const int point1 = skill1_min + (n^m) % ( skill1_max - skill1_min + 1 );

	//has skill 2?
	int skill2_index = -1, point2 = 0, skill2_min = 0, skill2_max = 0, skill2_name = 0;
	n = rnd( n );
	m = rnd( m );
	if( (n^m) % 100 >= StaticData::skill2_chance_table[ charm_type ] )
	{
		//skill 2
		m = rnd( m );
		skill2_index = m % num_skills2;
		skill2_name = skill2_table[ skill2_index * 3 ];
		skill2_min = skill2_table[ skill2_index * 3 + 1 ];
		skill2_max = skill2_table[ skill2_index * 3 + 2 ];

		//skill 2 point
		if( skill2_min > 0 ) //positive number
		{
			n = rnd( n );
			m = rnd( m );
			point2 = ( (n^m) % ( skill2_max - skill2_min + 1 ) ) + skill2_min;
		}
		else //check for negative
		{
			n = rnd( n );
			if( n % 2 == 1 ) //positive
			{
				n = rnd( n );
				m = rnd( m );
				point2 = (n^m) % skill2_max + 1;
			}
			else //negative
			{
				n = rnd( n );
				m = rnd( m );
				point2 = skill2_min + (n^m) % ( -skill2_min );
			}
		}		

		if( skill1_name == skill2_name )
		{
			skill2_min = skill2_max = point2 = 0;
			skill2_name = skill2_index = Ability::static_abilities.Count;
		}
	}

	const int slot_table = (int)Math::Floor( point1*10.0 / skill1_max + ( (point2 > 0) ? point2*10.0 / skill2_max : 0 ) );

	//slots
	n = rnd( n );
	int slot_roll = 0;
	if( n & 128 )
	{
		n = rnd( n );
		slot_roll = n % 100;
	}
	else
	{
		m = rnd( m );
		slot_roll = m % 100;
	}

	const int num_slots = GetNumSlots( charm_type, slot_table, slot_roll );

	if( num_slots != charm->num_slots )
		return false;

	if( point2 == 0 )
	{
		if( charm->abilities.Count == 2 ||
			skill1_name != charm->abilities[ 0 ]->ability->static_index ||
			point1 != charm->abilities[ 0 ]->amount )
			return false;
	}
	else
	{
		if( charm->abilities.Count != 2 ||
			skill1_name != charm->abilities[ 0 ]->ability->static_index ||
			skill2_name != charm->abilities[ 1 ]->ability->static_index ||
			point1 != charm->abilities[ 0 ]->amount ||
			point2 != charm->abilities[ 1 ]->amount )
			return false;
	}
	return true;
}

/*void GenerateCharm2( const unsigned charm_type, const unsigned table, int n, int m )
{
	//check charm_type < StaticData::skill1_table->Length
	array< unsigned char >^ skill1_table = StaticData::skill1_table[ charm_type ];
	const unsigned num_skills1 = skill1_table->Length / 3;
	//check charm_type < StaticData::skill2_table->Length
	array< signed char >^ skill2_table = StaticData::skill2_table[ charm_type ];
	const unsigned num_skills2 = skill2_table == nullptr ? 0 : skill2_table->Length / 3;

	//skill 1
	n = rnd( n );
	const int skill1_index = n % num_skills1;

	//skill 1 point
	n = rnd( n );
	m = rnd( m );
	const int skill1_name = skill1_table[ skill1_index * 3 ];
	const int skill1_min = skill1_table[ skill1_index * 3 + 1 ];
	const int skill1_max = skill1_table[ skill1_index * 3 + 2 ];
	const int point1 = skill1_min + (n^m) % ( skill1_max - skill1_min + 1 );

	//has skill 2?
	int skill2_index = -1, point2 = 0, skill2_min = 0, skill2_max = 0, skill2_name = 0;
	n = rnd( n );
	m = rnd( m );
	if( (n^m) % 100 >= StaticData::skill2_chance_table[ charm_type ] )
	{
		//skill 2
		m = rnd( m );
		skill2_index = m % num_skills2;
		skill2_name = skill2_table[ skill2_index * 3 ];
		skill2_min = skill2_table[ skill2_index * 3 + 1 ];
		skill2_max = skill2_table[ skill2_index * 3 + 2 ];

		//skill 2 point
		if( skill2_min > 0 ) //positive number
		{
			n = rnd( n );
			m = rnd( m );
			point2 = ( (n^m) % ( skill2_max - skill2_min + 1 ) ) + skill2_min;
		}
		else //check for negative
		{
			n = rnd( n );
			if( n % 2 == 1 ) //positive
			{
				n = rnd( n );
				m = rnd( m );
				point2 = (n^m) % skill2_max + 1;
			}
			else //negative
			{
				n = rnd( n );
				m = rnd( m );
				point2 = skill2_min + (n^m) % ( -skill2_min );
			}
		}

		if( skill1_name == skill2_name )
		{
			skill2_min = skill2_max = point2 = 0;
			skill2_name = skill2_index = Ability::static_abilities.Count;
		}
	}

	const int slot_table = (int)Math::Floor( point1*10.0 / skill1_max + ( (point2 > 0) ? point2*10.0 / skill2_max : 0 ) );

	//slots
	n = rnd( n );
	int slot_roll = 0;
	if( n & 128 )
	{
		n = rnd( n );
		slot_roll = n % 100;
	}
	else
	{
		m = rnd( m );
		slot_roll = m % 100;
	}

	const int num_slots = GetNumSlots( charm_type, slot_table, slot_roll );

	return;

	Charm^ charm = gcnew Charm( num_slots );
	charm->abilities.Add( gcnew AbilityPair( Ability::static_abilities[ skill1_name ], point1 ) );
	
	const unsigned charm_hash = charm->GetHash();
	const unsigned bits = ( 1 << table ) | ( 0x80000000 >> charm_type );
	if( CharmDatabase::hash_to_table->ContainsKey( charm_hash ) )
		CharmDatabase::hash_to_table[ charm_hash ] |= bits;
	else CharmDatabase::hash_to_table->Add( charm_hash, bits );
}*/

/*void GenerateCharm( const unsigned charm_type, const unsigned table, int n )
{
	//check charm_type < StaticData::skill1_table->Length
	array< unsigned char >^ skill1_table = StaticData::skill1_table[ charm_type ];
	const unsigned num_skills1 = skill1_table->Length / 3;
	//check charm_type < StaticData::skill2_table->Length
	array< signed char >^ skill2_table = StaticData::skill2_table[ charm_type ];
	const unsigned num_skills2 = skill2_table == nullptr ? 0 : skill2_table->Length / 3;

	//skill 1
	n = rnd( n );
	const int skill1_index = n % num_skills1;

	//skill 1 point
	n = rnd( n );
	const int skill1_name = skill1_table[ skill1_index * 3 ];
	const int skill1_min = skill1_table[ skill1_index * 3 + 1 ];
	const int skill1_max = skill1_table[ skill1_index * 3 + 2 ];
	const int point1 = skill1_min + n % ( skill1_max - skill1_min + 1 );

	//has skill 2?
	int skill2_index = -1, point2 = 0, skill2_min = 0, skill2_max = 0, skill2_name = 0;
	n = rnd( n );
	if( n % 100 >= StaticData::skill2_chance_table[ charm_type ] )
	{
		//skill 2
		n = rnd( n );
		skill2_index = n % num_skills2;
		skill2_name = skill2_table[ skill2_index * 3 ];
		skill2_min = skill2_table[ skill2_index * 3 + 1 ];
		skill2_max = skill2_table[ skill2_index * 3 + 2 ];

		//skill 2 point
		if( skill2_min > 0 ) //positive number
		{
			n = rnd( n );
			point2 = ( n % ( skill2_max - skill2_min + 1 ) ) + skill2_min;
		}
		else //check for negative
		{
			n = rnd( n );
			if( n % 2 == 1 ) //positive
			{
				n = rnd( n );
				point2 = n % skill2_max + 1;
			}
			else //negative
			{
				n = rnd( n );
				point2 = skill2_min + n % ( -skill2_min );
			}
		}	

		if( skill1_name == skill2_name )
		{
			skill2_min = skill2_max = point2 = 0;
			skill2_name = skill2_index = Ability::static_abilities.Count;
		}
	}

	const int slot_table = (int)Math::Floor( point1*10.0 / skill1_max + ( (point2 > 0) ? point2*10.0 / skill2_max : 0 ) );

	//slots
	n = rnd( n );
	const int num_slots = GetNumSlots( charm_type, slot_table, n % 100 );

	Charm^ charm = gcnew Charm( num_slots );
	charm->abilities.Add( gcnew AbilityPair( Ability::static_abilities[ skill1_name ], point1 ) );
	
	for( unsigned ct = charm_type; ct < 4; ++ct )
	{
		TableSlotDatum^ current_table = CharmDatabase::min_max[ table, ct ];
		TableSlotDatum^ all_table = CharmDatabase::min_max[ AllTable, ct ];

		MaxOut( current_table->max_single[ skill1_name, num_slots ], point1 );
		MaxOut( all_table->max_single[ skill1_name, num_slots ], point1 );
	}

	if( point2 != 0 )
	{
		charm->abilities.Add( gcnew AbilityPair( Ability::static_abilities[ skill2_name ], point2 ) );
	
		for( unsigned ct = charm_type; ct < 4; ++ct )
		{
			TableSlotDatum^ current_table = CharmDatabase::min_max[ table, ct ];
			TableSlotDatum^ all_table = CharmDatabase::min_max[ AllTable, ct ];

			MaxOut( current_table->max_single[ skill2_name, num_slots ], point2 );
			MaxOut( all_table->max_single[ skill2_name, num_slots ], point2 );
			
			List_t< Charm^ >^ list1 = current_table->two_skill_data[ num_slots ][ skill2_name, skill1_name ];
			Assert( list1 != nullptr, L"null list1" );
			Charm::AddToOptimalList( *list1, charm );

			List_t< Charm^ >^ list2 = all_table->two_skill_data[ num_slots ][ skill2_name, skill1_name ];
			Assert( list2 != nullptr, L"null list2" );
			Charm::AddToOptimalList( *list2, charm );
		}
	}
	
	const unsigned charm_hash = charm->GetHash();
	const unsigned bits = ( 1 << table ) | ( 0x80000000 >> charm_type );
	if( CharmDatabase::hash_to_table->ContainsKey( charm_hash ) )
		CharmDatabase::hash_to_table[ charm_hash ] |= bits;
	else CharmDatabase::hash_to_table->Add( charm_hash, bits );
}*/

array< unsigned char >^ LoadSlotTable( String^ filename )
{
	array< String^ >^ lines = IO::File::ReadAllLines( filename );
	array< unsigned char >^ result = gcnew array< unsigned char >( lines->Length * 3 - 3 );
	for( int i = 1, index = 0; i < lines->Length; ++i )
	{
		array< String^ >^ tokens = lines[ i ]->Split( ',' );
		for( int j = 1; j < tokens->Length; ++j )
			result[ index++ ] = (unsigned char)Convert::ToUInt16( tokens[ j ] );
	}
	return result;
}

template< typename T >
array< T >^ LoadSkillTable( String^ filename )
{
	array< String^ >^ lines = IO::File::ReadAllLines( filename );
	array< T >^ result = gcnew array< T >( lines->Length * 3 - 3 );
	for( int i = 1, index = 0; i < lines->Length; ++i )
	{
		array< String^ >^ tokens = lines[ i ]->Split( ',' );
		
		Ability^ ab = Ability::FindCharmAbility( tokens[ 0 ] );
		result[ index++ ] = ab->static_index;
		result[ index++ ] = (T)Convert::ToInt16( tokens[ 1 ] );
		result[ index++ ] = (T)Convert::ToInt16( tokens[ 2 ] );
	}
	return result;
}

void LoadCharmTableData()
{
	array< String^ >^ charm_names =
	{
		"mystery",
		"shining",
		"ancient",
		"distorted"
	};

	const unsigned NumCharmTypes = charm_names->Length;

	StaticData::skill1_table = gcnew array< array< unsigned char >^ >( NumCharmTypes );
	StaticData::skill2_table = gcnew array< array< signed char >^ >( NumCharmTypes );
	StaticData::slot_table = gcnew array< array< unsigned char >^ >( NumCharmTypes );

	

	for( unsigned i = 0; i < NumCharmTypes; ++i )
	{
		StaticData::slot_table[ i ] = LoadSlotTable( "Data/Charm Generation/" + charm_names[ i ] + "_slots.csv" );
		StaticData::skill1_table[ i ] = LoadSkillTable< unsigned char >( "Data/Charm Generation/" + charm_names[ i ] + "_skill1.csv" );
		if( i > 0 )
			StaticData::skill2_table[ i ] = LoadSkillTable< signed char >( "Data/Charm Generation/" + charm_names[ i ] + "_skill2.csv" );
	}
}

unsigned char GetMaxSingleSkill( const int index, const unsigned charm_type )
{
	unsigned char res = 0;
	for( int i = 0; i < StaticData::skill1_table[ charm_type ]->Length; i += 3 )
	{
		if( StaticData::skill1_table[ charm_type ][ i ] == index )
			res = Math::Max( res, StaticData::skill1_table[ charm_type ][ i + 2 ] );
	}
	if( StaticData::skill2_table[ charm_type ] )
	{
		for( int i = 0; i < StaticData::skill2_table[ charm_type ]->Length; i += 3 )
		{
			if( StaticData::skill2_table[ charm_type ][ i ] == index )
				res = Math::Max( res, (unsigned char)StaticData::skill2_table[ charm_type ][ i + 2 ] );
		}
	}
	return res;
}

void SetupSingleSkillMaxs()
{
	for( int i = 0; i < Ability::static_abilities.Count; ++i )
	{
		Ability::static_abilities[ i ]->max_vals1 = gcnew array< unsigned char >( 4 );
		for( unsigned charm_type = 0; charm_type < 4; charm_type++ )
		{
			Ability::static_abilities[ i ]->max_vals1[ charm_type ]	= GetMaxSingleSkill( Ability::static_abilities[ i ]->static_index, charm_type );
			if( charm_type > 0 )
				Ability::static_abilities[ i ]->max_vals1[ charm_type ] = Math::Max( Ability::static_abilities[ i ]->max_vals1[ charm_type ], Ability::static_abilities[ i ]->max_vals1[ charm_type - 1 ] );
		}
	}
}

void CreateTableSeedList()
{
	CharmDatabase::table_seed_list = gcnew array< List_t< unsigned short >^ >( CharmDatabase::table_seeds->Length );
	for( int i = 0; i < CharmDatabase::table_seeds->Length; ++i )
	{
		CharmDatabase::table_seed_list[ i ] = gcnew List_t< unsigned short >();

		int n = CharmDatabase::table_seeds[ i ];
		do 
		{
			CharmDatabase::table_seed_list[ i ]->Add( (unsigned short)(n & 0xFFFF) );
			n = rnd( n );
		}
		while( n != CharmDatabase::table_seeds[ i ] );

		CharmDatabase::table_seed_list[ i ]->Sort();
	}
}

int FindTable( const int n )
{
	for( int i = 0; i < CharmDatabase::table_seed_list->Length; ++i )
	{
		if( CharmDatabase::table_seed_list[i]->BinarySearch( n ) >= 0 )
			return i;
	}
	return -1;
}

void CharmDatabase::GenerateCharmTable()
{
	//mark all abilities as relevant for making optimal charm lists
	/*for each( Ability^ a in Ability::static_abilities )
	{
		a->relevant = true;
	}*/

	//set up databases
	//array< int >^ table_seeds = { 1, 15, 5, 13, 4, 3, 9, 12, 26, 18, 163, 401, 6, 2, 489, 802, 1203 };
	location_cache = gcnew Map_t< System::String^, CharmLocationDatum^ >();
	LoadCharmTableData();
	CreateTableSeedList();
	SetupSingleSkillMaxs();

	return;
	/*const unsigned num_skills = Ability::static_abilities.Count;
	const unsigned num_charm_types = StaticData::slot_table->Length;

	Assert( TotalTables == table_seeds->Length + 2, L"Incorrect number of table seeds." );
	min_max = gcnew array< TableSlotDatum^, 2 >( TotalTables, num_charm_types );
	hash_to_table = gcnew Map_t< unsigned, unsigned >();

	for( unsigned t = 0; t < TotalTables; ++t )
	{
		for( unsigned ct = 0; ct < num_charm_types; ++ct )
		{
			TableSlotDatum^ d = gcnew TableSlotDatum();
			min_max[ t, ct ] = d;

			d->max_single = gcnew array< signed char, 2 >( num_skills, 4 );

			if( t > 0 )
			{
				d->two_skill_data = gcnew array< array< List_t< Charm^ >^, 2 >^ >( 4 );
				for( unsigned s = 0; s < 4; ++s )
				{
					d->two_skill_data[ s ] = gcnew array< List_t< Charm^ >^, 2 >( num_skills, num_skills );
					for( unsigned skill1 = 1; skill1 < num_skills; ++skill1 )
					{
						for( unsigned skill2 = 0; skill2 < skill1; ++skill2 )
						{
							d->two_skill_data[ s ][ skill1, skill2 ] = gcnew List_t< Charm^ >();
							d->two_skill_data[ s ][ skill2, skill1 ] = d->two_skill_data[ s ][ skill1, skill2 ];
						}
					}
				}
			}
		}
	}*/

	//generate charms
	/*for( int t = 0; t < table_seeds->Length; ++t )
	{
		//unsigned charm_type = 2;
		for( unsigned charm_type = 0; charm_type < num_charm_types; ++charm_type )
		{
			int n = table_seeds[ t ];
			do
			{		
				GenerateCharm( charm_type, t+1, n );
				n = rnd( n );
			}
			while( n != table_seeds[ t ] );
		}
	}*/

	/*for( unsigned charm_type = 0; charm_type < num_charm_types; ++charm_type )
	{
		for( int t1 = 0; t1 < table_seeds->Length; ++t1 )
		{
			int n = table_seeds[ t1 ];
			do
			{
				for( int m = 1; m < 65363; ++m )
					GenerateCharm2( charm_type, t1+1, n, m );
				n = rnd( n );
			}
			while( n != table_seeds[ t1 ] );
		}
	}*/

	//turn abilities back to not-yet-relevant
	/*for each( Ability^ a in Ability::static_abilities )
	{
		a->relevant = false;
	}

	//calculate "don't know" tables
	for( unsigned ct = 0; ct < num_charm_types; ++ct )
	{
		array< signed char, 2 >::Copy( min_max[ 1, ct ]->max_single, 0, min_max[ 0, ct ]->max_single, 0, 4 * num_skills );

		for( unsigned t = 2; t < TotalTables-1; ++t )
		{
			//skip cursed tables
			if( t == 12 || t == 13  || t == 15 || t == 16 || t == 17 )
				continue;

			for( unsigned skill = 0; skill < num_skills; ++skill )
				for( unsigned slots = 0; slots < 4; ++slots )
				{
					const signed char m = min_max[ t, ct ]->max_single[ skill, slots ];
					MinOut( min_max[ 0, ct ]->max_single[ skill, slots ], m );
				}
		}
	}*/
}

#pragma endregion

/*int GetBitPositionFromRight( int v )
{
	int c;
	// if v is 1, returns 31.
	if (v & 0x1) 
	{
		// special case for odd v (assumed to happen half of the time)
		c = 0;
	}
	else
	{
		c = 1;
		if ((v & 0xffff) == 0) 
		{  
			v >>= 16;  
			c += 16;
		}
		if ((v & 0xff) == 0) 
		{  
			v >>= 8;  
			c += 8;
		}
		if ((v & 0xf) == 0) 
		{  
			v >>= 4;
			c += 4;
		}
		if ((v & 0x3) == 0) 
		{  
			v >>= 2;
			c += 2;
		}
		c -= v & 0x1;
	}
	return c;
}*/

/*int CheckCount( array< unsigned >^ counts )
{
	unsigned best = 0, num_best = 1;
	for( int i = 1; i < counts->Length; ++i )
	{
		if( counts[ i ] > counts[ best ] )
		{
			best = i;
			num_best = 1;
		}
		else if( counts[ i ] == counts[ best ] )
			num_best++;
	}
	if( num_best == 1 && counts[ best ] > 0 )
		return best + 1;
	return -1;
}*/

bool IsAutoguardCharm( Charm^ charm )
{
	return charm->num_slots == 0 &&
		charm->abilities.Count == 1 &&
		charm->abilities[ 0 ]->amount == 10 &&
		charm->abilities[ 0 ]->ability == Ability::auto_guard;
}

bool CanFind2SkillCharm( Charm^ charm )
{
	if( charm->abilities.Count < 2 )
		return false;

	const unsigned start = ( charm->num_slots == 3 ) ? 2 : 1;

	for( unsigned charm_type = start; charm_type < 4; ++charm_type )
	{
		int skill1_table_index = -1;
		for( skill1_table_index = 0; skill1_table_index < StaticData::skill1_table[ charm_type ]->Length; skill1_table_index += 3 )
		{
			if( StaticData::skill1_table[ charm_type ][ skill1_table_index ] == charm->abilities[ 0 ]->ability->static_index )
				break;
		}
		if( skill1_table_index >= StaticData::skill1_table[ charm_type ]->Length )
			continue;

		int skill2_table_index = -1;
		for( skill2_table_index = 0; skill2_table_index < StaticData::skill2_table[ charm_type ]->Length; skill2_table_index += 3 )
		{
			if( StaticData::skill2_table[ charm_type ][ skill2_table_index ] == charm->abilities[ 1 ]->ability->static_index )
				break;
		}
		if( skill2_table_index >= StaticData::skill2_table[ charm_type ]->Length )
			continue;

		const unsigned num1 = StaticData::skill1_table[ charm_type ]->Length / 3;
		const unsigned num2 = StaticData::skill2_table[ charm_type ]->Length / 3;

		for( int n = skill1_table_index / 3; n < 65363; n += num1 )
		{
			for( int m = skill2_table_index / 3; m < 65363; m += num2 )
			{
				const int initm = reverse_rnd(reverse_rnd(m == 0 ? num2 : m));
				if( CanGenerate2SkillCharm( charm_type, n, initm, charm ) )
					return true;
			}
		}
	}
	return false;
}

/*bool CanFindCharm( int t1, array< int >^ table_seeds, Charm^ charm )
{
	if( charm->abilities.Count < 1 )
		return true;

	for( unsigned charm_type = 0; charm_type < 4; ++charm_type )
	{
		int skill1_table_index;
		for( skill1_table_index = 0; skill1_table_index < StaticData::skill1_table[ charm_type ]->Length; skill1_table_index += 3 )
		{
			if( StaticData::skill1_table[ charm_type ][ skill1_table_index ] == charm->abilities[ 0 ]->ability->static_index )
				break;
		}
		if( skill1_table_index >= StaticData::skill1_table[ charm_type ]->Length )
			continue;

		int skill2_table_index = -1;
		if( charm->abilities.Count == 2 )
		{
			if( StaticData::skill2_table[ charm_type ] == nullptr )
				continue;
			for( skill2_table_index = 0; skill2_table_index < StaticData::skill2_table[ charm_type ]->Length; skill2_table_index += 3 )
			{
				if( StaticData::skill2_table[ charm_type ][ skill2_table_index ] == charm->abilities[ 1 ]->ability->static_index )
					break;
			}
			if( skill2_table_index >= StaticData::skill2_table[ charm_type ]->Length )
				continue;
		}
		
		const unsigned num_skills1 = StaticData::skill1_table[ charm_type ]->Length / 3;
		int n = table_seeds[ t1 ];
		do 
		{
			const int n2 = rnd( n );
			if( n % num_skills1 == skill1_table_index / 3 )
			{
				const int n3 = rnd( n2 );
				const int n4 = rnd( n3 );
				const int n5 = rnd( n4 );
				const int n6 = rnd( n5 );
				const int n7 = rnd( n6 );
				const int skill1_min = StaticData::skill1_table[ charm_type ][ skill1_table_index + 1 ];
				const int skill1_max = StaticData::skill1_table[ charm_type ][ skill1_table_index + 2 ];
				const int skill1_range = skill1_max - skill1_min + 1;

				for( int m = 1; m < 65363; ++m )
				{
					if( (n2^m) % skill1_range + skill1_min == charm->abilities[ 0 ]->amount )
					{
						const int m2 = rnd( m );
						const bool skill2_rolled = (n3^m2) % 100 >= StaticData::skill2_chance_table[ charm_type ];
						if( charm->abilities.Count == 1 && !skill2_rolled )
						{
							//1 skill
							int slot_roll;
							if( n4 & 128 )
								slot_roll = n5 % 100;
							else
							{
								const int m3 = rnd( m2 );
								slot_roll = m3 % 100;
							}
							const int slot_table = (int)Math::Floor( charm->abilities[ 0 ]->amount*10.0 / skill1_max );
							const int slots = GetNumSlots( charm_type, slot_table, slot_roll );
							if( slots == charm->num_slots )
								return true;
						}
						else if( charm->abilities.Count == 2 && skill2_rolled )
						{
							//2 skills
							const int m3 = rnd( m2 );
							const unsigned num_skills2 = StaticData::skill2_table[ charm_type ]->Length / 3;
							if( m3 % num_skills2 == skill2_table_index / 3 )
							{
								const int skill2_min = StaticData::skill2_table[ charm_type ][ skill2_table_index + 1 ];
								const int skill2_max = StaticData::skill2_table[ charm_type ][ skill2_table_index + 2 ];
								const int skill2_range = skill2_max - skill2_min + 1;
								const int m4 = rnd( m3 );

								if( skill2_min > 0 )
								{
									if( (n4^m4) % skill2_range + skill2_min == charm->abilities[ 1 ]->amount )
									{
										int slot_roll;
										if( n5 & 128 )
											slot_roll = n6 % 100;
										else
										{
											const int m5 = rnd( m4 );
											slot_roll = m5 % 100;
										}
										const int slot_table = (int)Math::Floor( charm->abilities[ 0 ]->amount*10.0 / skill1_max + Math::Max( charm->abilities[ 1 ]->amount, 0 )*10.0 / skill2_max );
										const int slots = GetNumSlots( charm_type, slot_table, slot_roll );
										if( slots == charm->num_slots )
											return true;
									}
								}
								else
								{
									int skill2_roll;
									if( n4 % 2 == 1 ) //positive
									{
										skill2_roll = (n5^m4) % skill2_max + 1;
									}
									else //negative
									{
										skill2_roll = skill2_min + (n5^m4) % ( -skill2_min );
									}
									if( skill2_roll == charm->abilities[ 1 ]->amount )
									{
										int slot_roll;
										if( n6 & 128 )
											slot_roll = n7 % 100;
										else
										{
											const int m5 = rnd( m4 );
											slot_roll = m5 % 100;
										}
										const int slot_table = (int)Math::Floor( charm->abilities[ 0 ]->amount*10.0 / skill1_max + Math::Max( charm->abilities[ 1 ]->amount, 0 )*10.0 / skill2_max );
										const int slots = GetNumSlots( charm_type, slot_table, slot_roll );
										if( slots == charm->num_slots )
											return true;
									}
								}
							}
						}
					}
				}
			}
			n = n2;
		} while( n != table_seeds[ t1 ] );
	}
	return false;


	for( int t2 = 0; t2 < table_seeds->Length; ++t2 )
	{
		for( unsigned charm_type = 1; charm_type < 3; ++charm_type )
		{
			int n = table_seeds[ t1 ];
			do
			{
				int m = table_seeds[ t2 ];
				do
				{
					if( GenerateCharm3( charm_type, t1+1, n, m, charm ) )
						return true;
					m = rnd( m );
				}
				while( m != table_seeds[ t2 ] );
				n = rnd( n );
			}
			while( n != table_seeds[ t1 ] );
		}
	}
	return false;
}*/

bool CharmDatabase::CharmIsLegal( Charm^ charm )
{
	if( charm->num_slots >= 4 )
		return false;
	else if( charm->abilities.Count == 0 || IsAutoguardCharm( charm ) )
		return true;
	else if( charm->abilities.Count == 1 )
	{
		for( unsigned c = charm->num_slots; c < 4; ++c )
			if( charm->abilities[0]->amount <= charm->abilities[0]->ability->max_vals1[c] )
				return true;
	}
	else if( charm->abilities.Count == 2 )
	{
		return CanFind2SkillCharm( charm );
	}
	return false;
}

void FindTwoSkillCharms( array< List_t< unsigned >^ >^ charms, const int n0, const int m0, const int num1, const int num2, const unsigned t )
{
	for( int n = n0; n < 65363; n += num1 )
	{
		for( int m = m0; m < 65363; m += num2 )
		{
			const int initm = reverse_rnd(reverse_rnd(m == 0 ? num2 : m));
			if( TryTwoSkillCharm( t, n, initm, charms ) )
				return;
		}
	}
}

void FindTwoSkillCharms( array< List_t< unsigned >^ >^ charms, const unsigned index1, const unsigned index2, const unsigned max_charm_type )
{
	for( unsigned charm_type = 1; charm_type <= max_charm_type; ++charm_type )
	{
		int skill1_table_index;
		for( skill1_table_index = 0; skill1_table_index < StaticData::skill1_table[ charm_type ]->Length; skill1_table_index += 3 )
		{
			if( StaticData::skill1_table[ charm_type ][ skill1_table_index ] == index1 )
				break;
		}
		if( skill1_table_index >= StaticData::skill1_table[ charm_type ]->Length )
			continue;

		int skill2_table_index = -1;
		for( skill2_table_index = 0; skill2_table_index < StaticData::skill2_table[ charm_type ]->Length; skill2_table_index += 3 )
		{
			if( StaticData::skill2_table[ charm_type ][ skill2_table_index ] == index2 )
				break;
		}
		if( skill2_table_index >= StaticData::skill2_table[ charm_type ]->Length )
			continue;

		const unsigned num_skills1 = StaticData::skill1_table[ charm_type ]->Length / 3;
		const unsigned num_skills2 = StaticData::skill2_table[ charm_type ]->Length / 3;

		FindTwoSkillCharms( charms, skill1_table_index / 3, skill2_table_index / 3, num_skills1, num_skills2, charm_type );
	}
}

bool ContainsBetterCharm( List_t< Charm^ >^ charms, const int p1, const int p2, Ability^ ab1, Ability^ ab2 )
{
	for( int i = 0; i < charms->Count; ++i )
	{
		if( charms[i]->abilities[0]->ability == ab1 &&
			charms[i]->abilities[1]->ability == ab2 &&
			charms[i]->abilities[0]->amount >= p1 &&
			charms[i]->abilities[1]->amount >= p2 )
			return true;
	}
	return false;
}

bool ContainsBetterCharm( List_t< Charm^ >^ charms, Charm^ charm )
{
	for each( Charm^ c in charms )
	{
		if( c->abilities[0]->ability == charm->abilities[0]->ability &&
			c->abilities[0]->amount >= charm->abilities[0]->amount )
			return true;
		if( c->abilities.Count == 2 &&
			c->abilities[1]->ability == charm->abilities[0]->ability &&
			c->abilities[1]->amount >= charm->abilities[0]->amount )
			return true;
	}
	return false;
}

void GetDoubleSkillCharms( List_t< Charm^ >^ list, List_t< Skill^ >% skills, const unsigned max_charm_type )
{
	const unsigned max_slots = Math::Min( 3u, max_charm_type + 1 );
	
	array< List_t< Charm^ >^ >^ two_skills = gcnew array< List_t< Charm^ >^ >( max_slots + 1 );
	for( unsigned k = 0; k <= max_slots; ++k )
		two_skills[ k ] = gcnew List_t< Charm^ >();

	for( int i = 1; i < skills.Count; ++i )
	{
		Ability^ ab1 = skills[ i ]->ability;
		for( int j = 0; j < i; ++j )
		{
			Ability^ ab2 = skills[ j ]->ability;

			array< List_t< unsigned >^ >^ charms = gcnew array< List_t< unsigned >^ >( max_slots + 1 );
			for( unsigned k = 0; k <= max_slots; ++k )
				charms[ k ] = gcnew List_t< unsigned >();

			FindTwoSkillCharms( charms, ab1->static_index, ab2->static_index, max_charm_type );

			for( unsigned k = 0; k <= max_slots; ++k )
			{
				for( unsigned l = charms[k]->Count; l --> 0; )
				{
					const int p1 = charms[k][l] >> 16;
					const int p2 = charms[k][l] & 0xFFFF;
					if( !ContainsBetterCharm( two_skills[k], p1, p2, ab1, ab2 ) )
					{
						Charm^ c = gcnew Charm( k );
						c->abilities.Add( gcnew AbilityPair( ab1, p1 ) );
						c->abilities.Add( gcnew AbilityPair( ab2, p2 ) );

						two_skills[k]->Add( c );
					}
				}
			}
		}
	}

	for each( Charm^ c in list )
	{
		if( !ContainsBetterCharm( two_skills[ c->num_slots ], c ) )
			two_skills[ c->num_slots ]->Add( c );
	}

	list->Clear();
	for( unsigned k = 0; k <= max_slots; ++k )
		list->AddRange( two_skills[ k ] );
}

void GetSingleSkillCharms( List_t< Charm^ >^ list, List_t< Skill^ >% skills, const unsigned max_charm_type )
{
	const unsigned max_slots = Math::Min( 3u, max_charm_type + 1 );
	for each( Skill^ skill in skills )
	{
		if( skill->ability->max_vals1 == nullptr )
			continue;

		for( unsigned slots = 1; slots <= max_slots; ++slots )
		{
			Charm^ ct = gcnew Charm;
			ct->num_slots = slots;
			ct->abilities.Add( gcnew AbilityPair( skill->ability, skill->ability->max_vals1[ max_charm_type ] ) );
			list->Add( ct );
		}
	}
}

void AddSlotOnlyCharms( List_t< Charm^ >^ res, Query^ query, const unsigned max_charm_type )
{
	bool have[ 4 ] = { false, false, false, false };
	for each( Charm^ charm in res )
	{
		have[ charm->num_slots ] = true;
	}

	for( unsigned slots = Math::Min( 3u, max_charm_type + 1 ); slots > 0; --slots )
	{
		if( !have[ slots ] )
		{
			res->Add( gcnew Charm( slots ) );
			break;
		}
	}
}

List_t< Charm^ >^ CharmDatabase::GetCharms( Query^ query, const bool use_two_skill_charms )
{
	List_t< Charm^ >^ res = gcnew List_t< Charm^ >;
	const unsigned max_charm_type = CalcMaxCharmType( query );

	GetSingleSkillCharms( res, query->skills, max_charm_type );

	if( use_two_skill_charms && max_charm_type > 0 )
		GetDoubleSkillCharms( res, query->skills, max_charm_type );

	AddSlotOnlyCharms( res, query, max_charm_type );

	return res;
}

CharmLocationDatum^ CharmDatabase::FindCharmLocations( Charm^ charm )
{
	CharmLocationDatum^ result = gcnew CharmLocationDatum();
	result->table = gcnew array< unsigned, 2 >( 17, 4 );
	result->examples = gcnew array< System::String^ >( 17 );
	const unsigned limit = 64000;


	unsigned num_found = 0;
	for( unsigned charm_type = 0; charm_type < 4; ++charm_type )
	{
		if( charm->num_slots > charm_type + 1 )
			continue;

		int skill1_table_index = -1;
		for( skill1_table_index = 0; skill1_table_index < StaticData::skill1_table[ charm_type ]->Length; skill1_table_index += 3 )
		{
			if( StaticData::skill1_table[ charm_type ][ skill1_table_index ] == charm->abilities[0]->ability->static_index &&
				StaticData::skill1_table[ charm_type ][ skill1_table_index + 2 ] >= charm->abilities[0]->amount )
				break;
		}

		const unsigned num1 = StaticData::skill1_table[ charm_type ]->Length / 3;

		if( charm->abilities.Count == 1 )
		{
			int skill2_table_index = -1;
			if( StaticData::skill2_table[ charm_type ] )
			{
				for( skill2_table_index = 0; skill2_table_index < StaticData::skill2_table[ charm_type ]->Length; skill2_table_index += 3 )
				{
					if( StaticData::skill2_table[ charm_type ][ skill2_table_index ] == charm->abilities[0]->ability->static_index &&
						StaticData::skill2_table[ charm_type ][ skill2_table_index + 2 ] >= charm->abilities[0]->amount )
						break;
				}
			}
			if( skill1_table_index >= StaticData::skill1_table[ charm_type ]->Length &&
				( skill2_table_index < 0 || skill2_table_index >= StaticData::skill2_table[ charm_type ]->Length ) )
				continue;

			const unsigned num2 = StaticData::skill2_table[ charm_type ]->Length / 3;

			for( int t = 0; t < table_seeds->Length; ++t )
			{
				if( t == 10 || t == 11  || t == 14 || t == 15 || t == 16 )
				{
					/*int n = table_seeds[ t ];
					do 
					{
						for( int m = 1; m < 65363; ++m )
						{
							if( result->table[t, charm_type] >= limit )
								break;

							String^ str = CanGenerateCharm1( charm_type, n, m, charm );
							if( str )
							{
								if( result->table[t, charm_type]++ == 0 )
									result->examples[ t ] = str;
								else
									result->examples[ t ] = nullptr;

								if( num_found++ == 0 )
									result->example = str;
								else result->example = nullptr;
							}
						}
						if( result->table[t, charm_type] >= limit )
							break;
						n = rnd( n );
					}
					while( n != table_seeds[ t ] );*/
				}
				else
				{
					result->table[t, charm_type] = limit;
				}
			}
		}
		else
		{
			if( skill1_table_index >= StaticData::skill1_table[ charm_type ]->Length )
				continue;

			int skill2_table_index = -1;
			for( skill2_table_index = 0; skill2_table_index < StaticData::skill2_table[ charm_type ]->Length; skill2_table_index += 3 )
			{
				if( StaticData::skill2_table[ charm_type ][ skill2_table_index ] == charm->abilities[1]->ability->static_index &&
					StaticData::skill2_table[ charm_type ][ skill2_table_index + 2 ] >= charm->abilities[1]->amount )
					break;
			}
			if( skill2_table_index >= StaticData::skill2_table[ charm_type ]->Length )
				continue;

			const unsigned num2 = StaticData::skill2_table[ charm_type ]->Length / 3;

			for( int n = skill1_table_index / 3; n < 65363; n += num1 )
			{
				const int table = FindTable( n );
				for( int m = skill2_table_index / 3; m < 65363; m += num2 )
				{
					const int initm = reverse_rnd(reverse_rnd(m == 0 ? num2 : m));
					String^ str = CanGenerateCharm2( charm_type, n, initm, charm );
					if( str )
					{
						if( result->table[table, charm_type]++ == 0 )
							result->examples[ table ] = str;
						else
							result->examples[ table ] = nullptr;

						if( num_found++ == 0 )
							result->example = str;
						else result->example = nullptr;
					}
				}
			}
		}
	}
	return result;
}

#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

#include "humblenet_attributes.h"
#include "humblenet_types_internal.h"

class AttributeSetFixture {
protected:
	humblenet_AttributeMap* p;
	humblenet::AttributeMap* m;
public:
	AttributeSetFixture()
	{
		p = humblenet_attributes_new();
		m = reinterpret_cast<humblenet::AttributeMap*>(p);
	}

	~AttributeSetFixture()
	{
		m = nullptr;
		humblenet_attributes_free( p );
		p = nullptr;
	}
};

SCENARIO( "humblenet_attributes_new" )
{
	THEN( "it returns a new attribute set pointer" ) {
		auto p = humblenet_attributes_new();

		REQUIRE( p != nullptr );

		humblenet_attributes_free( p );
	}
}

SCENARIO_METHOD( AttributeSetFixture, "humblenet_attributes_set" )
{
	GIVEN( "the key is not set" ) {
		humblenet_attributes_set( p, "key", "value" );

		THEN( "it sets an attribute on the map" ) {
			REQUIRE( m->count( "key" ) == 1 );
			CHECK( m->at( "key" ) == "value" );
		}
	}

	GIVEN( "the key already is set" ) {
		m->emplace( "key", "old value" );

		humblenet_attributes_set( p, "key", "value" );

		THEN( "it sets an attribute on the map" ) {
			REQUIRE( m->count( "key" ) == 1 );
			CHECK( m->at( "key" ) == "value" );
		}
	}
}

SCENARIO_METHOD( AttributeSetFixture, "humblenet_attributes_contains" )
{
	GIVEN( "the key is not set" ) {
		THEN( "it returns false" ) {
			CHECK_FALSE( humblenet_attributes_contains( p, "key" ));
		}
	}

	GIVEN( "the key is set" ) {
		humblenet_attributes_set( p, "key", "value" );

		THEN( "it returns true" ) {
			CHECK( humblenet_attributes_contains( p, "key" ));
		}
	}
}

SCENARIO_METHOD( AttributeSetFixture, "humblenet_attributes_unset" )
{
	GIVEN( "the attribute is not set" ) {
		CHECK( m->empty());
		humblenet_attributes_unset( p, "key" );

		THEN( "it does nothing" ) {
			CHECK( m->empty());
		}
	}

	GIVEN( "the attribute is set" ) {
		m->emplace( "key", "value" );
		humblenet_attributes_unset( p, "key" );

		THEN( "it removes the key" ) {
			CHECK( m->count( "key" ) == 0 );
		}
	}
}

SCENARIO_METHOD( AttributeSetFixture, "humblenet_attributes_size" )
{
	GIVEN( "the attribute set is empty" ) {
		CHECK( m->empty());

		THEN( "it returns 0" ) {
			CHECK( humblenet_attributes_size( p ) == 0 );
		}
	}

	GIVEN( "an attribute is set" ) {
		m->emplace( "key", "value" );

		THEN( "it returns 1" ) {
			CHECK( humblenet_attributes_size( p ) == 1 );
		}
	}
}

SCENARIO_METHOD( AttributeSetFixture, "humblenet_attributes_get" )
{
	GIVEN( "the attribute is not set" ) {
		CHECK( m->empty());
		char buff[10];

		THEN( "it returns false" ) {
			CHECK_FALSE( humblenet_attributes_get( p, "key", sizeof( buff ), buff ));
		}
	}

	GIVEN( "the attribute is set" ) {
		m->emplace( "key", "value" );

		char buff[10];

		THEN( "it returns true" ) {
			CHECK( humblenet_attributes_get( p, "key", sizeof( buff ), buff ));
		}

		THEN( "it populates the buffer" ) {
			humblenet_attributes_get( p, "key", sizeof( buff ), buff );
			CHECK( std::string(buff) == "value" );
		}
	}
}

SCENARIO_METHOD( AttributeSetFixture, "humblenet_attributes_iterate" )
{
	GIVEN( "an empty attribute set" ) {
		CHECK( m->empty());

		THEN( "it returns null" ) {
			CHECK( humblenet_attributes_iterate( p ) == nullptr );
		}
	}

	GIVEN( "a non-empty attribute set" ) {
		m->emplace( "key", "value" );

		THEN( "it returns an iterator pointer" ) {
			auto it = humblenet_attributes_iterate( p );

			CHECK( it != nullptr );

			humblenet_attributes_iterate_free( it );
		}
	}
}

SCENARIO_METHOD( AttributeSetFixture, "humblenet_attributes_get_key" )
{
	GIVEN( "a valid iterator" ) {
		m->emplace( "key", "value" );

		auto it = humblenet_attributes_iterate( p );

		THEN( "it sets the buffer to the key name" ) {
			char buff[10];

			humblenet_attributes_iterate_get_key( it, sizeof( buff ), buff );

			CHECK( std::string(buff) == "key" );
		}

		humblenet_attributes_iterate_free( it );
	}
}

SCENARIO_METHOD( AttributeSetFixture, "humblenet_attributes_next" )
{
	GIVEN( "only one item in the map" ) {
		m->emplace( "key", "value" );

		auto it = humblenet_attributes_iterate( p );

		THEN( "it returns null" ) {
			CHECK( humblenet_attributes_iterate_next( it ) == nullptr );
		}
	}

	GIVEN( "multiple items in the map" ) {
		m->emplace( "key", "value" );
		m->emplace( "other", "value 2" );

		auto it = humblenet_attributes_iterate( p );

		THEN( "it returns the next iterator" ) {
			auto nit = humblenet_attributes_iterate_next( it );

			CHECK( nit != nullptr );

			humblenet_attributes_iterate_free( nit );
		}
	}
}
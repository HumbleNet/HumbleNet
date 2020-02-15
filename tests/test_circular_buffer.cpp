#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

#include "../src/humblenet/src/circular_buffer.h"

struct Entry {
	int value;

	Entry() : value( 0 ) {}

	explicit Entry(int v) : value( v ) {}
};

SCENARIO( "Circular buffer handling" )
{
	GIVEN( "An empty buffer" ) {
		CircularBuffer<Entry> q( 5 );
		int e_id = 0;
		Entry e;

		REQUIRE( q.used() == 0 );
		REQUIRE( q.available() == 5 );

		WHEN( "popping an entry" ) {
			THEN( "it should return false" ) {
				REQUIRE_FALSE( q.pop( e ));
			}
		}

		WHEN( "an entry is added" ) {
			REQUIRE( q.push( Entry( ++e_id )));

			THEN( "the size and capacity change" ) {
				REQUIRE( q.used() == 1 );
				REQUIRE( q.available() == 4 );
			}
		}
	}

	GIVEN( "a full queue" ) {
		CircularBuffer<Entry> q( 5 );
		int e_id = 0;
		Entry e;

		REQUIRE( q.push( Entry( ++e_id )));
		REQUIRE( q.push( Entry( ++e_id )));
		REQUIRE( q.push( Entry( ++e_id )));
		REQUIRE( q.push( Entry( ++e_id )));
		REQUIRE( q.push( Entry( ++e_id )));

		THEN( "it should be full" ) {
			REQUIRE( q.used() == 5 );
			REQUIRE( q.available() == 0 );
		}

		WHEN( "pushing an entry" ) {
			THEN( "it should fail adding new entries" ) {
				REQUIRE_FALSE( q.push( Entry( ++e_id )));
			}
		}

		WHEN( "popping an entry" ) {
			THEN( "it pops the first entry" ) {
				REQUIRE( q.pop( e ));

				REQUIRE( e.value == 1 );
			}
		}
	}

	GIVEN( "a partially full queue" ) {
		CircularBuffer<Entry> q( 5 );
		int e_id = 0;
		Entry e;

		REQUIRE( q.push( Entry( ++e_id )));
		REQUIRE( q.push( Entry( ++e_id )));
		REQUIRE( q.push( Entry( ++e_id )));

		THEN( "it should be partially full" ) {
			REQUIRE( q.used() == 3 );
			REQUIRE( q.available() == 2 );
		}

		WHEN( "pushing an entry" ) {
			THEN( "it adds new entries" ) {
				REQUIRE( q.push( Entry( ++e_id )));
			}
		}

		WHEN( "popping an entry" ) {
			THEN( "it pops the first entry" ) {
				REQUIRE( q.pop( e ));

				REQUIRE( e.value == 1 );
			}
		}
	}
}

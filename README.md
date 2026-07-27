Taken from my hobby project https://git.ovcefilda.cz/filip.bartos/radiant. Licensed under the unlicense, see `LICENSE` for more information.

# C++26 Tagged union

This is a header only tagged union implementation using C++26. It tracks the currently in-use type using compile time reflection.
I have written some tests but there are a few more compile time checks that could be added to catch some errors early.

### Example use

```cpp
enum pipeline_type_t
{
	// Each tagged union NEEDS to have a none value annotated with tagged_union_none_value_t.
	PIPELINE_TYPE_none [[=tagged_union_none_value_t{}]],
	PIPELINE_TYPE_graphics,
	PIPELINE_TYPE_compute,
	PIPELINE_TYPE_mesh,
};

struct pipeline_t
{
	struct [[=pipeline_type_t(PIPELINE_TYPE_graphics)]] graphics_t
	{
		void* m_vertex;
		void* m_fragment;
	};

	struct [[=pipeline_type_t(PIPELINE_TYPE_compute)]] compute_t
	{
		void* m_compute;
	};

	struct [[=pipeline_type_t(PIPELINE_TYPE_mesh)]] mesh_t
	{
		void* m_task;
		void* m_mesh;
		void* m_fragment;
	};
};

using pipeline_union_t = tagged_union_t<pipeline_t, pipeline_type_t>;

int main(int argc, char* argv[])
{
	pipeline_union_t pipeline;

	// Constructs pipeline_t::graphics_t
	pipeline.set(PIPELINE_TYPE_graphics);

	// Returns pointer to pipeline_t::graphics_t as it is in use right now.
	pipeline.get<pipeline_t::graphics_t>();
	// Returns nullptr as we're using graphics_t, not compute_t.
	pipeline.get<pipeline_t::compute_t>();

	// Destructs pipeline_t::graphics_t
}

```

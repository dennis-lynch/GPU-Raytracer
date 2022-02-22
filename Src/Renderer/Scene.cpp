#include "Scene.h"

#include <ctype.h>
#include <stdio.h>

#include "Core/IO.h"

#include "Assets/OBJLoader.h"
#include "Assets/PLYLoader.h"
#include "Assets/Mitsuba/MitsubaLoader.h"
#include "Assets/PBRTLoader.h"

#include "Material.h"

#include "Util/Util.h"
#include "Util/StringUtil.h"

Scene::Scene() : camera(Math::deg_to_rad(85.0f)) {
	LinearAllocator<MEGABYTES(4)> allocator;

	for (int i = 0; i < cpu_config.scene_filenames.size(); i++) {
		const String & scene_filename = cpu_config.scene_filenames[i];

		StringView file_extension = Util::get_file_extension(scene_filename.view());
		if (file_extension.is_empty()) {
			IO::print("ERROR: File '{}' has no file extension, cannot deduce file format!\n"_sv, scene_filename);
			IO::exit(1);
		}

		if (file_extension == "obj") {
			add_mesh(scene_filename, asset_manager.add_mesh_data(scene_filename, &allocator, OBJLoader::load));
		} else if (file_extension == "ply") {
			add_mesh(scene_filename, asset_manager.add_mesh_data(scene_filename, &allocator, PLYLoader::load));
		} else if (file_extension == "xml") {
			MitsubaLoader::load(scene_filename, &allocator, *this);
		} else if (file_extension == "pbrt") {
			PBRTLoader::load(scene_filename, &allocator, *this);
		} else {
			IO::print("ERROR: '{}' file format is not supported!\n"_sv, file_extension);
			IO::exit(1);
		}
	}

	sky.load(cpu_config.sky_filename);
}

Mesh & Scene::add_mesh(String name, MeshDataHandle mesh_data_handle, MaterialHandle material_handle) {
	return meshes.emplace_back(std::move(name), mesh_data_handle, material_handle, *this);
}

void Scene::calc_properties() {
	has_diffuse    = false;
	has_plastic    = false;
	has_dielectric = false;
	has_conductor  = false;
	has_lights     = false;

	// Check properties of the Scene, so we know which kernels are required
	for (int i = 0; i < asset_manager.materials.size(); i++) {
		switch (asset_manager.materials[i].type) {
			case Material::Type::DIFFUSE:    has_diffuse    = true; break;
			case Material::Type::PLASTIC:    has_plastic    = true; break;
			case Material::Type::DIELECTRIC: has_dielectric = true; break;
			case Material::Type::CONDUCTOR:  has_conductor  = true; break;
			case Material::Type::LIGHT:      has_lights     = true; break;
		}
	}
}

void Scene::update(float delta) {
	for (int i = 0; i < meshes.size(); i++) {
		meshes[i].update();
	}
}

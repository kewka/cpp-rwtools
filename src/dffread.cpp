#include <cmath>
#include "renderware.h"
using namespace std;

namespace rw {

char *filename;

/*
 * Clump
 */

void Clump::read(ifstream& rw)
{
	HeaderInfo header;

	// TODO: this is only a quick and dirty fix for uv anim dicts
	header.read(rw);
	if (header.type != CHUNK_CLUMP) {
		rw.seekg(header.length, ios::cur);
		header.read(rw);
		if (header.type != CHUNK_CLUMP)
			return;
	}

	READ_HEADER(CHUNK_STRUCT);
	uint32 numAtomics = readUInt32(rw);
	uint32 numLights;
	if (header.length == 0xC) {
		numLights = readUInt32(rw);
		rw.seekg(4, ios::cur); /* camera count, unused in gta */
	} else {
		numLights = 0;
	}
	atomicList.resize(numAtomics);

	READ_HEADER(CHUNK_FRAMELIST);

	READ_HEADER(CHUNK_STRUCT);
	uint32 numFrames = readUInt32(rw);
	frameList.resize(numFrames);
	for (uint32 i = 0; i < numFrames; i++)
		frameList[i].readStruct(rw);
	for (uint32 i = 0; i < numFrames; i++)
		frameList[i].readExtension(rw);

	READ_HEADER(CHUNK_GEOMETRYLIST);

	READ_HEADER(CHUNK_STRUCT);
	uint32 numGeometries = readUInt32(rw);
	geometryList.resize(numGeometries);
	for (uint32 i = 0; i < numGeometries; i++)
		geometryList[i].read(rw);

	/* read atomics */
	for (uint32 i = 0; i < numAtomics; i++) {
		atomicList[i].read(rw);
//		cout << i << " " << frameList[atomicList[i].frameIndex].name
//		        << " " << atomicList[i].geometryIndex << endl;
	}

	/* skip lights */
	for (uint32 i = 0; i < numLights; i++) {
		READ_HEADER(CHUNK_STRUCT);
		rw.seekg(header.length, ios::cur);
		READ_HEADER(CHUNK_LIGHT);
		rw.seekg(header.length, ios::cur);
	}

	readExtension(rw);
}

void Clump::readExtension(ifstream &rw)
{
	HeaderInfo header;

	READ_HEADER(CHUNK_EXTENSION);

	uint32 end = rw.tellg();
	end += header.length;

	while (rw.tellg() < end) {
		header.read(rw);
		switch (header.type) {
		case CHUNK_COLLISIONMODEL:
			rw.seekg(header.length, ios::cur);
			break;
		default:
			rw.seekg(header.length, ios::cur);
			break;
		}
	}
}

void Clump::dump(bool detailed)
{
	string ind = "";
	cout << ind << "Clump {\n";
	ind += "  ";
	cout << ind << "numAtomics: " << atomicList.size() << endl;

	cout << endl << ind << "FrameList {\n";
	ind += "  ";
	cout << ind << "numFrames: " << frameList.size() << endl;
	for (uint32 i = 0; i < frameList.size(); i++)
		frameList[i].dump(i, ind, detailed);
//		dumpFrame(frameList[i], i, ind);
	ind = ind.substr(0, ind.size()-2);
	cout << ind << "}\n";

	cout << endl << ind << "GeometryList {\n";
	ind += "  ";
	cout << ind << "numGeometries: " << geometryList.size() << endl;
	for (uint32 i = 0; i < geometryList.size(); i++)
		geometryList[i].dump(i, ind, detailed);
//		dumpGeometry(geometryList[i], i, ind);

	ind = ind.substr(0, ind.size()-2);
	cout << ind << "}\n\n";

	for (uint32 i = 0; i < atomicList.size(); i++)
		atomicList[i].dump(i, ind, detailed);
//		dumpAtomic(atomicList[i], i, ind);

	ind = ind.substr(0, ind.size()-2);
	cout << ind << "}\n";
}

void Clump::clear(void)
{
	atomicList.resize(0);
	geometryList.resize(0);
	frameList.resize(0);
}

/*
 * Atomic
 */

void Atomic::read(ifstream &rw)
{
	HeaderInfo header;

	READ_HEADER(CHUNK_ATOMIC);

	READ_HEADER(CHUNK_STRUCT);
	frameIndex = readUInt32(rw);
	geometryIndex = readUInt32(rw);
	rw.seekg(8, ios::cur);	// constant

	readExtension(rw);
}

void Atomic::readExtension(ifstream &rw)
{
	HeaderInfo header;

	READ_HEADER(CHUNK_EXTENSION);

	uint32 end = rw.tellg();
	end += header.length;

	while (rw.tellg() < end) {
		header.read(rw);
		switch (header.type) {
		case CHUNK_RIGHTTORENDER:
			hasRightToRender = true;
			rightToRenderVal1 = readUInt32(rw);
			rightToRenderVal2 = readUInt32(rw);
			break;
		case CHUNK_PARTICLES:
			hasParticles = true;
			particlesVal = readUInt32(rw);
			break;
		case CHUNK_MATERIALEFFECTS:
			hasMaterialFx = true;
			materialFxVal = readUInt32(rw);
			break;
		case CHUNK_PIPELINESET:
			hasPipelineSet = true;
			pipelineSetVal = readUInt32(rw);
			break;
		default:
			rw.seekg(header.length, ios::cur);
			break;
		}
	}
}

void Atomic::dump(uint32 index, string ind, bool detailed)
{
	cout << ind << "Atomic " << index << " {\n";
	ind += "  ";

	cout << ind << "frameIndex: " << frameIndex << endl;
	cout << ind << "geometryIndex: " << geometryIndex << endl;

	if (hasRightToRender) {
		cout << hex;
		cout << ind << "rightToRenderVal1: " << rightToRenderVal1<<endl;
		cout << ind << "rightToRenderVal2: " << rightToRenderVal2<<endl;
		cout << dec;
	}

	if (hasParticles)
		cout << ind << "particlesVal: " << particlesVal << endl;

	if (hasPipelineSet)
		cout << ind << "pipelineSetVal: " << pipelineSetVal << endl;

	if (hasMaterialFx)
		cout << ind << "materialFxVal: " << materialFxVal << endl;

	ind = ind.substr(0, ind.size()-2);
	cout << ind << "}\n";
}

Atomic::Atomic(void)
{
	hasRightToRender = false;
	hasParticles = false;
	hasMaterialFx = false;
	hasPipelineSet = false;
}

/*
 * Frame
 */

// only reads part of the frame struct
void Frame::readStruct(ifstream &rw)
{
	rw.read((char *) rotationMatrix, 9*sizeof(float32));
	rw.read((char *) position, 3*sizeof(float32));
	parent = readInt32(rw);
	rw.seekg(4, ios::cur);	// matrix creation flag, unused
}

void Frame::readExtension(ifstream &rw)
{
	HeaderInfo header;

	READ_HEADER(CHUNK_EXTENSION);

	uint32 end = rw.tellg();
	end += header.length;

	while (rw.tellg() < end) {
		header.read(rw);
		switch (header.type) {
		case CHUNK_FRAME:
		{
			char *buffer = new char[header.length+1];
			rw.read(buffer, header.length);
			buffer[header.length] = '\0';
			name = buffer;
			delete[] buffer;
			break;
		}
		case CHUNK_HANIM:
			hasHAnim = true;
			hAnimUnknown1 = readUInt32(rw);;
			hAnimBoneId = readInt32(rw);;
			hAnimBoneCount = readUInt32(rw);;
			if (hAnimBoneCount != 0) {
				hAnimUnknown2 = readUInt32(rw);;
				hAnimUnknown3 = readUInt32(rw);;
			}
			for (uint32 i = 0; i < hAnimBoneCount; i++) {
				hAnimBoneIds.push_back(readInt32(rw));
				hAnimBoneNumbers.push_back(readUInt32(rw));
				hAnimBoneTypes.push_back(readUInt32(rw));
			}
			break;
		default:
			rw.seekg(header.length, ios::cur);
			break;
		}
	}
}

void Frame::dump(uint32 index, string ind, bool detailed)
{
	cout << ind << "Frame " << index << " {\n";
	ind += "  ";

	cout << ind << "rotationMatrix: ";
	for (uint32 i = 0; i < 9; i++)
		cout << rotationMatrix[i] << " ";
	cout << endl;

	cout << ind << "position: ";
	for (uint32 i = 0; i < 3; i++)
		cout << position[i] << " ";
	cout << endl;
	cout << ind << "parent: " << parent << endl;

	cout << ind << "name: " << name << endl;

	// TODO: hanim

	ind = ind.substr(0, ind.size()-2);
	cout << ind << "}\n";
}

Frame::Frame(void)
{
	hasHAnim = false;
	hAnimBoneCount = 0;
}

/*
 * Geometry
 */

void Geometry::read(ifstream &rw)
{
	HeaderInfo header;

	READ_HEADER(CHUNK_GEOMETRY);

	READ_HEADER(CHUNK_STRUCT);
	flags = readUInt16(rw);
	numUVs = readUInt8(rw);
	hasNativeGeometry = readUInt8(rw);
	uint32 triangleCount = readUInt32(rw);
	vertexCount = readUInt32(rw);
	rw.seekg(4, ios::cur); /* number of morph targets, uninteresting */
	// skip light info
	if (header.version == GTA3_1 || header.version == GTA3_2 ||
	    header.version == GTA3_3 || header.version == GTA3_4 ||
	    header.version == VCPS2) {
		rw.seekg(12, ios::cur);
	}

	if (!hasNativeGeometry) {
		if (flags & FLAGS_PRELIT) {
			vertexColors.resize(4*vertexCount);
			rw.read((char *) (&vertexColors[0]),
			         4*vertexCount*sizeof(uint8));
		}
		if (flags & FLAGS_TEXTURED) {
			texCoords[0].resize(2*vertexCount);
			rw.read((char *) (&texCoords[0][0]),
			         2*vertexCount*sizeof(float32));
		}
		if (flags & FLAGS_TEXTURED2) {
			for (uint32 i = 0; i < numUVs; i++) {
				texCoords[i].resize(2*vertexCount);
				rw.read((char *) (&texCoords[i][0]),
					 2*vertexCount*sizeof(float32));
			}
		}
		faces.resize(4*triangleCount);
		rw.read((char *) (&faces[0]), 4*triangleCount*sizeof(uint16));
	}

	/* morph targets, only 1 in gta */
	rw.read((char *)(boundingSphere), 4*sizeof(float32));
	//hasPositions = (flags & FLAGS_POSITIONS) ? 1 : 0;
	hasPositions = readUInt32(rw);
	hasNormals = readUInt32(rw);
	// need to recompute:
	hasPositions = 1;
	hasNormals = (flags & FLAGS_NORMALS) ? 1 : 0;

	if (!hasNativeGeometry) {
		vertices.resize(3*vertexCount);
		rw.read((char *) (&vertices[0]), 3*vertexCount*sizeof(float32));
		if (flags & FLAGS_NORMALS) {
			normals.resize(3*vertexCount);
			rw.read((char *) (&normals[0]),
				 3*vertexCount*sizeof(float32));
		}
	}

	READ_HEADER(CHUNK_MATLIST);

	READ_HEADER(CHUNK_STRUCT);
	uint32 numMaterials = readUInt32(rw);
	rw.seekg(numMaterials*4, ios::cur);	// constant

	materialList.resize(numMaterials);
	for (uint32 i = 0; i < numMaterials; i++)
		materialList[i].read(rw);

	readExtension(rw);
}

void Geometry::readExtension(ifstream &rw)
{
	HeaderInfo header;

	READ_HEADER(CHUNK_EXTENSION);

	uint32 end = rw.tellg();
	end += header.length;

	while (rw.tellg() < end) {
		header.read(rw);
		switch (header.type) {
		case CHUNK_BINMESH: {
			faceType = readUInt32(rw);
			uint32 numSplits = readUInt32(rw);
			numIndices = readUInt32(rw);
			splits.resize(numSplits);
			for (uint32 i = 0; i < numSplits; i++) {
				uint32 numIndices = readUInt32(rw);
				splits[i].matIndex = readUInt32(rw);
				splits[i].indices.resize(numIndices);
				if (!hasNativeGeometry)
					for (uint32 j = 0; j < numIndices; j++)
						splits[i].indices[j] = 
					          readUInt32(rw);
			}
			break;
		} case CHUNK_NATIVEDATA: {
			uint32 beg = rw.tellg();
			rw.seekg(0x0c, ios::cur);
			uint32 platform = readUInt32(rw);
			rw.seekg(beg, ios::beg);
			if (platform == PLATFORM_PS2)
				readPs2NativeData(rw);
			else if (platform == PLATFORM_XBOX)
				readXboxNativeData(rw);
			else
				cout << "unknown platform " << platform << endl;
			break;
		}
		case CHUNK_MESHEXTENSION: {
			hasMeshExtension = true;
			meshExtension = new MeshExtension;
			meshExtension->unknown = readUInt32(rw);
			readMeshExtension(rw);
			break;
		} case CHUNK_NIGHTVERTEXCOLOR: {
			hasNightColors = true;
			nightColorsUnknown = readUInt32(rw);
			if (nightColors.size() != 0) {
				// native data also has them, so skip
				rw.seekg(header.length - sizeof(uint32),
				          ios::cur);
			} else {
				if (nightColorsUnknown != 0) {
				/* TODO: could be better */
					nightColors.resize(header.length-4);
					rw.read((char *)
					   (&nightColors[0]), header.length-4);
				}
			}
			break;
		} case CHUNK_MORPH: {
			hasMorph = true;
			/* always 0 */
			readUInt32(rw);
			break;
		} case CHUNK_SKIN: {
			if (hasNativeGeometry) {
				uint32 beg = rw.tellg();
				rw.seekg(0x0c, ios::cur);
				uint32 platform = readUInt32(rw);
				rw.seekg(beg, ios::beg);
				uint32 end = beg+header.length;
				if (platform == PLATFORM_PS2) {
					hasSkin = true;
					readPs2NativeSkin(rw);
				} else if (platform == PLATFORM_XBOX) {
					hasSkin = true;
					readXboxNativeSkin(rw);
				} else {
					cout << "skin: unknown platform "
					     << platform << endl;
					rw.seekg(header.length, ios::cur);
				}
			} else {
				hasSkin = true;
				boneCount = readUInt8(rw);
				specialIndexCount = readUInt8(rw);
				unknown1 = readUInt8(rw);
				unknown2 = readUInt8(rw);

				specialIndices.resize(specialIndexCount);
				rw.read((char *) (&specialIndices[0]),
					 specialIndexCount*sizeof(uint8));

				vertexBoneIndices.resize(vertexCount);
				rw.read((char *) (&vertexBoneIndices[0]),
					 vertexCount*sizeof(uint32));

				vertexBoneWeights.resize(vertexCount*4);
				rw.read((char *) (&vertexBoneWeights[0]),
					 vertexCount*4*sizeof(float32));

				inverseMatrices.resize(boneCount*16);
				for (uint32 i = 0; i < boneCount; i++) {
					// skip 0xdeaddead
					if (specialIndexCount == 0)
						rw.seekg(4, ios::cur);
					rw.read((char *)
						 (&inverseMatrices[i*0x10]),
						 0x10*sizeof(float32));
				}
				// skip some zeroes
				if (specialIndexCount != 0)
					rw.seekg(0x0C, ios::cur);
			}
			break;
		}
		case CHUNK_ADCPLG:
			/* only sa ps2, ignore (not very interesting anyway) */
			rw.seekg(header.length, ios::cur);
			break;
		case CHUNK_2DFX:
			rw.seekg(header.length, ios::cur);
			break;
		default:
			rw.seekg(header.length, ios::cur);
			break;
		}
	}
}

void Geometry::readMeshExtension(ifstream &rw)
{
	if (meshExtension->unknown == 0)
		return;
	rw.seekg(0x4, ios::cur);
	uint32 vertexCount = readUInt32(rw);
	rw.seekg(0xC, ios::cur);
	uint32 faceCount = readUInt32(rw);
	rw.seekg(0x8, ios::cur);
	uint32 materialCount = readUInt32(rw);
	rw.seekg(0x10, ios::cur);

	/* vertices */
	meshExtension->vertices.resize(3*vertexCount);
	rw.read((char *) (&meshExtension->vertices[0]),
		 3*vertexCount*sizeof(float32));
	/* tex coords */
	meshExtension->texCoords.resize(2*vertexCount);
	rw.read((char *) (&meshExtension->texCoords[0]),
		 2*vertexCount*sizeof(float32));
	/* vertex colors */
	meshExtension->vertexColors.resize(4*vertexCount);
	rw.read((char *) (&meshExtension->vertexColors[0]),
		 4*vertexCount*sizeof(uint8));
	/* faces */
	meshExtension->faces.resize(3*faceCount);
	rw.read((char *) (&meshExtension->faces[0]),
		 3*faceCount*sizeof(uint16));
	/* material assignments */
	meshExtension->assignment.resize(faceCount);
	rw.read((char *) (&meshExtension->assignment[0]),
		 faceCount*sizeof(uint16));

	meshExtension->textureName.resize(materialCount);
	meshExtension->maskName.resize(materialCount);
	char buffer[0x20];
	for (uint32 i = 0; i < materialCount; i++) {
		rw.read(buffer, 0x20);
		meshExtension->textureName[i] = buffer;
		rw.read(buffer, 0x20);
		meshExtension->maskName[i] = buffer;
		meshExtension->unknowns.push_back(readFloat32(rw));
		meshExtension->unknowns.push_back(readFloat32(rw));
		meshExtension->unknowns.push_back(readFloat32(rw));
	}
}

bool Geometry::isDegenerateFace(uint32 i, uint32 j, uint32 k)
{
	if (vertices[i*3+0] == vertices[j*3+0] &&
	    vertices[i*3+1] == vertices[j*3+1] &&
	    vertices[i*3+2] == vertices[j*3+2])
		return true;
	if (vertices[i*3+0] == vertices[k*3+0] &&
	    vertices[i*3+1] == vertices[k*3+1] &&
	    vertices[i*3+2] == vertices[k*3+2])
		return true;
	if (vertices[j*3+0] == vertices[k*3+0] &&
	    vertices[j*3+1] == vertices[k*3+1] &&
	    vertices[j*3+2] == vertices[k*3+2])
		return true;
	return false;
}

// native console data doesn't have face information, use this to generate it
void Geometry::generateFaces(void)
{
	faces.clear();

	if (faceType == FACETYPE_STRIP)
		for (uint32 i = 0; i < splits.size(); i++) {
			Split &s = splits[i];
			for (uint32 j = 0; j < s.indices.size()-2; j++) {
				if (isDegenerateFace(s.indices[j+0],
				    s.indices[j+1], s.indices[j+2]))
					continue;
				faces.push_back(s.indices[j+1 + (j%2)]);
				faces.push_back(s.indices[j+0]);
				faces.push_back(s.matIndex);
				faces.push_back(s.indices[j+2 - (j%2)]);
			}
		}
	else
		for (uint32 i = 0; i < splits.size(); i++) {
			Split &s = splits[i];
			for (uint32 j = 0; j < s.indices.size()-2; j+=3) {
				faces.push_back(s.indices[j+1]);
				faces.push_back(s.indices[j+0]);
				faces.push_back(s.matIndex);
				faces.push_back(s.indices[j+2]);
			}
		}
}

// these hold the (temporary) cleaned up data
vector<float32> vertices_new;
vector<float32> normals_new;
vector<float32> texCoords_new[8];
vector<uint8> vertexColors_new;
vector<uint8> nightColors_new;
vector<uint32> vertexBoneIndices_new;
vector<float32> vertexBoneWeights_new;

// used only by Geometry::cleanUp()
// adds new temporary vertex if it isn't already in the list
// and returns the new index of that vertex
uint32 Geometry::addTempVertexIfNew(uint32 index)
{
	// return if we already have the vertex
	for (uint32 i = 0; i < vertices_new.size()/3; i++) {
		if (vertices_new[i*3+0] != vertices[index*3+0] ||
		    vertices_new[i*3+1] != vertices[index*3+1] ||
		    vertices_new[i*3+2] != vertices[index*3+2])
			continue;
		if (flags & FLAGS_NORMALS) {
			if (normals_new[i*3+0] != normals[index*3+0] ||
			    normals_new[i*3+1] != normals[index*3+1] ||
			    normals_new[i*3+2] != normals[index*3+2])
				continue;
		}
		if (flags & FLAGS_TEXTURED || flags & FLAGS_TEXTURED2) {
			for (uint32 j = 0; j < numUVs; j++)
				if (texCoords_new[j][i*2+0] !=
				                    texCoords[j][index*2+0] ||
				    texCoords_new[j][i*2+1] !=
				                    texCoords[j][index*2+1])
					goto cont;
		}
		if (flags & FLAGS_PRELIT) {
			if (vertexColors_new[i*4+0]!=vertexColors[index*4+0] ||
			    vertexColors_new[i*4+1]!=vertexColors[index*4+1] ||
			    vertexColors_new[i*4+2]!=vertexColors[index*4+2] ||
			    vertexColors_new[i*4+3]!=vertexColors[index*4+3])
				continue;
		}
		if (hasNightColors) {
			if (nightColors_new[i*4+0] != nightColors[index*4+0] ||
			    nightColors_new[i*4+1] != nightColors[index*4+1] ||
			    nightColors_new[i*4+2] != nightColors[index*4+2] ||
			    nightColors_new[i*4+3] != nightColors[index*4+3])
				continue;
		}
		if (hasSkin) {
			if (vertexBoneIndices_new[i]!=vertexBoneIndices[index])
				continue;

			if (vertexBoneWeights_new[i*4+0] !=
			                      vertexBoneWeights[index*4+0] ||
			    vertexBoneWeights_new[i*4+1] !=
			                      vertexBoneWeights[index*4+1] ||
			    vertexBoneWeights_new[i*4+2] !=
			                      vertexBoneWeights[index*4+2] ||
			    vertexBoneWeights_new[i*4+3] !=
			                      vertexBoneWeights[index*4+3])
				continue;
		}
		// this is only reached when the vertex is already in the list
		return i;

		cont: ;
	}

	// else add the vertex
	vertices_new.push_back(vertices[index*3+0]);
	vertices_new.push_back(vertices[index*3+1]);
	vertices_new.push_back(vertices[index*3+2]);
	if (flags & FLAGS_NORMALS) {
		normals_new.push_back(normals[index*3+0]);
		normals_new.push_back(normals[index*3+1]);
		normals_new.push_back(normals[index*3+2]);
	}
	if (flags & FLAGS_TEXTURED || flags & FLAGS_TEXTURED2) {
		for (uint32 j = 0; j < numUVs; j++) {
			texCoords_new[j].push_back(texCoords[j][index*2+0]);
			texCoords_new[j].push_back(texCoords[j][index*2+1]);
		}
	}
	if (flags & FLAGS_PRELIT) {
		vertexColors_new.push_back(vertexColors[index*4+0]);
		vertexColors_new.push_back(vertexColors[index*4+1]);
		vertexColors_new.push_back(vertexColors[index*4+2]);
		vertexColors_new.push_back(vertexColors[index*4+3]);
	}
	if (hasNightColors) {
		nightColors_new.push_back(nightColors[index*4+0]);
		nightColors_new.push_back(nightColors[index*4+1]);
		nightColors_new.push_back(nightColors[index*4+2]);
		nightColors_new.push_back(nightColors[index*4+3]);
	}
	if (hasSkin) {
		vertexBoneIndices_new.push_back(vertexBoneIndices[index]);

		vertexBoneWeights_new.push_back(vertexBoneWeights[index*4+0]);
		vertexBoneWeights_new.push_back(vertexBoneWeights[index*4+1]);
		vertexBoneWeights_new.push_back(vertexBoneWeights[index*4+2]);
		vertexBoneWeights_new.push_back(vertexBoneWeights[index*4+3]);
	}
	return vertices_new.size()/3 - 1;
}

// removes duplicate vertices (only useful with ps2 meshes)
void Geometry::cleanUp(void)
{
	vertices_new.clear();
	normals_new.clear();
	vertexColors_new.clear();
	nightColors_new.clear();
	for (uint32 i = 0; i < 8; i++)
		texCoords_new[i].clear();
	vertexBoneIndices_new.clear();
	vertexBoneWeights_new.clear();

	vector<uint32> newIndices;

	// create new vertex list
	for (uint32 i = 0; i < vertices.size()/3; i++)
		newIndices.push_back(addTempVertexIfNew(i));

	vertices = vertices_new;
	if (flags & FLAGS_NORMALS)
		normals = normals_new;
	if (flags & FLAGS_TEXTURED || flags & FLAGS_TEXTURED2)
		for (uint32 j = 0; j < numUVs; j++)
			texCoords[j] = texCoords_new[j];
	if (flags & FLAGS_PRELIT)
		vertexColors = vertexColors_new;
	if (hasNightColors)
		nightColors = nightColors_new;
	if (hasSkin) {
		vertexBoneIndices = vertexBoneIndices_new;
		vertexBoneWeights = vertexBoneWeights_new;
	}

	// correct indices
	for (uint32 i = 0; i < splits.size(); i++)
		for (uint32 j = 0; j < splits[i].indices.size(); j++)
			splits[i].indices[j] = newIndices[splits[i].indices[j]];

	for (uint32 i = 0; i < faces.size()/4; i++) {
		faces[i*4+0] = newIndices[faces[i*4+0]];
		faces[i*4+1] = newIndices[faces[i*4+1]];
		faces[i*4+3] = newIndices[faces[i*4+3]];
	}
}

void Geometry::dump(uint32 index, string ind, bool detailed)
{
	cout << ind << "Geometry " << index << " {\n";
	ind += "  ";

	cout << ind << "flags: " << hex << flags << endl;
	cout << ind << "numUVs: " << dec << numUVs << endl;
	cout << ind << "hasNativeGeometry: " << hasNativeGeometry << endl;
	cout << ind << "triangleCount: " << faces.size()/4 << endl;
	cout << ind << "vertexCount: " << vertexCount << endl << endl;;

	if (flags & FLAGS_PRELIT) {
		cout << ind << "vertexColors {\n";
		ind += "  ";
		if (!detailed)
			cout << ind << "skipping\n";
		else
			for (uint32 i = 0; i < vertexColors.size()/4; i++)
				cout << ind << int(vertexColors[i*4+0])<< ", "
					<< int(vertexColors[i*4+1]) << ", "
					<< int(vertexColors[i*4+2]) << ", "
					<< int(vertexColors[i*4+3]) << endl;
		ind = ind.substr(0, ind.size()-2);
		cout << ind << "}\n\n";
	}

	if (flags & FLAGS_TEXTURED) {
		cout << ind << "texCoords {\n";
		ind += "  ";
		if (!detailed)
			cout << ind << "skipping\n";
		else
			for (uint32 i = 0; i < texCoords[0].size()/2; i++)
				cout << ind << texCoords[0][i*2+0]<< ", "
					<< texCoords[0][i*2+1] << endl;
		ind = ind.substr(0, ind.size()-2);
		cout << ind << "}\n\n";
	}
	if (flags & FLAGS_TEXTURED2) {
		for (uint32 j = 0; j < numUVs; j++) {
		cout << ind << "texCoords " << j << " {\n";
		ind += "  ";
		if (!detailed)
			cout << ind << "skipping\n";
		else
			for (uint32 i = 0; i < texCoords[j].size()/2; i++)
				cout << ind << texCoords[j][i*2+0]<< ", "
					<< texCoords[j][i*2+1] << endl;
		ind = ind.substr(0, ind.size()-2);
		cout << ind << "}\n\n";
		}
	}

	cout << ind << "faces {\n";
	ind += "  ";
	if (!detailed)
		cout << ind << "skipping\n";
	else
		for (uint32 i = 0; i < faces.size()/4; i++)
			cout << ind << faces[i*4+0] << ", "
			            << faces[i*4+1] << ", "
			            << faces[i*4+2] << ", "
			            << faces[i*4+3] << endl;
	ind = ind.substr(0, ind.size()-2);
	cout << ind << "}\n\n";

	cout << ind << "boundingSphere: ";
	for (uint32 i = 0; i < 4; i++)
		cout << boundingSphere[i] << " ";
	cout << endl << endl;
	cout << ind << "hasPositions: " << hasPositions << endl;
	cout << ind << "hasNormals: " << hasNormals << endl;

	cout << ind << "vertices {\n";
	ind += "  ";
	if (!detailed)
		cout << ind << "skipping\n";
	else
		for (uint32 i = 0; i < vertices.size()/3; i++)
			cout << ind << vertices[i*3+0] << ", "
			            << vertices[i*3+1] << ", "
			            << vertices[i*3+2] << endl;
	ind = ind.substr(0, ind.size()-2);
	cout << ind << "}\n";

	if (flags & FLAGS_NORMALS) {
		cout << ind << "normals {\n";
		ind += "  ";
		if (!detailed)
			cout << ind << "skipping\n";
		else
			for (uint32 i = 0; i < normals.size()/3; i++)
				cout << ind << normals[i*3+0] << ", "
					    << normals[i*3+1] << ", "
					    << normals[i*3+2] << endl;
		ind = ind.substr(0, ind.size()-2);
		cout << ind << "}\n";
	}

	cout << endl << ind << "MaterialList {\n";
	ind += "  ";
	cout << ind << "numMaterials: " << materialList.size() << endl;
	for (uint32 i = 0; i < materialList.size(); i++)
		materialList[i].dump(i, ind, detailed);
	ind = ind.substr(0, ind.size()-2);
	cout << ind << "}\n";

	ind = ind.substr(0, ind.size()-2);
	cout << ind << "}\n";
}

Geometry::Geometry(void)
{
	hasMorph = false;
	hasMeshExtension = false;
	meshExtension = 0;
	hasSkin = false;
	hasNightColors = false;
}

Geometry::Geometry(const Geometry &orig)
: flags(orig.flags), numUVs(orig.numUVs),
  hasNativeGeometry(orig.hasNativeGeometry), vertexCount(orig.vertexCount),
  faces(orig.faces), vertexColors(orig.vertexColors),
  hasPositions(orig.hasPositions), hasNormals(orig.hasNormals),
  vertices(orig.vertices), normals(orig.normals),
  materialList(orig.materialList), faceType(orig.faceType),
  numIndices(orig.numIndices), splits(orig.splits), hasSkin(orig.hasSkin),
  boneCount(orig.boneCount), specialIndexCount(orig.specialIndexCount),
  unknown1(orig.unknown1), unknown2(orig.unknown2),
  specialIndices(orig.specialIndices),
  vertexBoneIndices(orig.vertexBoneIndices),
  vertexBoneWeights(orig.vertexBoneWeights),
  inverseMatrices(orig.inverseMatrices),
  hasMeshExtension(orig.hasMeshExtension), hasNightColors(orig.hasNightColors),
  nightColorsUnknown(orig.nightColorsUnknown), nightColors(orig.nightColors),
  hasMorph(orig.hasMorph)
{
	if (orig.meshExtension)
		meshExtension = new MeshExtension(*orig.meshExtension);
	else
		meshExtension = 0;

	for (uint32 i = 0; i < 8; i++)
		texCoords[i] = orig.texCoords[i];
	for (uint32 i = 0; i < 4; i++)
		boundingSphere[i] = orig.boundingSphere[i];
}

Geometry &Geometry::operator=(const Geometry &that)
{
	if (this != &that) {
		flags = that.flags;
		numUVs = that.numUVs;
		hasNativeGeometry = that.hasNativeGeometry;

		vertexCount = that.vertexCount;
		faces = that.faces;
		vertexColors = that.vertexColors;
		for (uint32 i = 0; i < 8; i++)
			texCoords[i] = that.texCoords[i];

		for (uint32 i = 0; i < 4; i++)
			boundingSphere[i] = that.boundingSphere[i];

		hasPositions = that.hasPositions;
		hasNormals = that.hasNormals;
		vertices = that.vertices;
		normals = that.normals;
		materialList = that.materialList;

		faceType = that.faceType;
		numIndices = that.numIndices;
		splits = that.splits;

		hasSkin = that.hasSkin;
		boneCount = that.boneCount;
		specialIndexCount = that.specialIndexCount;
		unknown1 = that.unknown1;
		unknown2 = that.unknown2;
		specialIndices = that.specialIndices;
		vertexBoneIndices = that.vertexBoneIndices;
		vertexBoneWeights = that.vertexBoneWeights;
		inverseMatrices = that.inverseMatrices;

		hasMeshExtension = that.hasMeshExtension;
		delete meshExtension;
		meshExtension = 0;
		if (that.meshExtension)
			meshExtension = new MeshExtension(*that.meshExtension);

		hasNightColors = that.hasNightColors;
		nightColorsUnknown = that.nightColorsUnknown;
		nightColors = that.nightColors;

		hasMorph = that.hasMorph;
	}
	return *this;
}

Geometry::~Geometry(void)
{
	delete meshExtension;
	meshExtension = 0;
}


/*
 * Material
 */

void Material::read(ifstream &rw)
{
	HeaderInfo header;

	READ_HEADER(CHUNK_MATERIAL);

	READ_HEADER(CHUNK_STRUCT);
	flags = readUInt32(rw);
	rw.read((char *) (color), 4*sizeof(uint8));
	unknown = readUInt32(rw);;
	hasTex = readInt32(rw);
	rw.read((char *) (surfaceProps), 3*sizeof(float32));

	if (hasTex)
		texture.read(rw);

	readExtension(rw);
}

void Material::readExtension(ifstream &rw)
{
	HeaderInfo header;

	READ_HEADER(CHUNK_EXTENSION);

	uint32 end = rw.tellg();
	end += header.length;

	while (rw.tellg() < end) {
		header.read(rw);
		switch (header.type) {
		case CHUNK_RIGHTTORENDER:
			hasRightToRender = true;
			rightToRenderVal1 = readUInt32(rw);
			rightToRenderVal2 = readUInt32(rw);
			break;
		case CHUNK_MATERIALEFFECTS: {
			hasMatFx = true;
			matFx = new MatFx;
			matFx->type = readUInt32(rw);
			switch (matFx->type) {
			case MATFX_BUMPMAP: {
				rw.seekg(4, ios::cur); // also MATFX_BUMPMAP
				matFx->bumpCoefficient = readFloat32(rw);

				matFx->hasTex1 = readUInt32(rw);
				if (matFx->hasTex1)
					matFx->tex1.read(rw);

				matFx->hasTex2 = readUInt32(rw);
				if (matFx->hasTex2)
					matFx->tex2.read(rw);

				rw.seekg(4, ios::cur); // 0
				break;
			} case MATFX_ENVMAP: {
				rw.seekg(4, ios::cur); // also MATFX_ENVMAP
				matFx->envCoefficient = readFloat32(rw);

				matFx->hasTex1 = readUInt32(rw);
				if (matFx->hasTex1)
					matFx->tex1.read(rw);

				matFx->hasTex2 = readUInt32(rw);
				if (matFx->hasTex2)
					matFx->tex2.read(rw);

				rw.seekg(4, ios::cur); // 0
				break;
			} case MATFX_BUMPENVMAP: {
				rw.seekg(4, ios::cur); // MATFX_BUMPMAP
				matFx->bumpCoefficient = readFloat32(rw);
				matFx->hasTex1 = readUInt32(rw);
				if (matFx->hasTex1)
					matFx->tex1.read(rw);
				// needs to be 0, tex2 will be used
				rw.seekg(4, ios::cur);

				rw.seekg(4, ios::cur); // MATFX_ENVMPMAP
				matFx->envCoefficient = readFloat32(rw);
				// needs to be 0, tex1 is already used
				rw.seekg(4, ios::cur);
				matFx->hasTex2 = readUInt32(rw);
				if (matFx->hasTex2)
					matFx->tex2.read(rw);
				break;
			} case MATFX_DUAL: {
				rw.seekg(4, ios::cur); // also MATFX_DUAL
				matFx->srcBlend = readUInt32(rw);
				matFx->destBlend = readUInt32(rw);

				matFx->hasDualPassMap = readUInt32(rw);
				if (matFx->hasDualPassMap)
					matFx->dualPassMap.read(rw);
				rw.seekg(4, ios::cur); // 0
				break;
			} case MATFX_UVTRANSFORM: {
				rw.seekg(4, ios::cur);//also MATFX_UVTRANSFORM
				rw.seekg(4, ios::cur); // 0
				break;
			} case MATFX_DUALUVTRANSFORM: {
				// never observed in gta
				break;
			} default:
				break;
			}
			break;
		} case CHUNK_REFLECTIONMAT:
			hasReflectionMat = true;
			reflectionChannelAmount[0] = readFloat32(rw);
			reflectionChannelAmount[1] = readFloat32(rw);
			reflectionChannelAmount[2] = readFloat32(rw);
			reflectionChannelAmount[3] = readFloat32(rw);
			reflectionIntensity = readFloat32(rw);
			rw.seekg(4, ios::cur);
			break;
		case CHUNK_SPECULARMAT: {
			hasSpecularMat = true;
			specularLevel = readFloat32(rw);
			uint32 len = header.length - sizeof(float32) - 4;
			char *name = new char[len];
			rw.read(name, len);
			specularName = name;
			rw.seekg(4, ios::cur);
			delete[] name;
			break;
		}
		case CHUNK_UVANIMDICT:
			rw.seekg(header.length, ios::cur);
			break;
		default:
			rw.seekg(header.length, ios::cur);
			break;
		}
	}
}

void Material::dump(uint32 index, string ind, bool detailed)
{
	cout << ind << "Material " << index << " {\n";
	ind += "  ";

	// unused
//	cout << ind << "flags: " << hex << flags << endl;
	cout << ind << "color: " << dec << int(color[0]) << " "
	                         << int(color[1]) << " "
	                         << int(color[2]) << " "
	                         << int(color[3]) << endl;
	// unused
//	cout << ind << "unknown: " << hex << unknown << endl;
	cout << ind << "surfaceProps: " << surfaceProps[0] << " "
	                                << surfaceProps[1] << " "
	                                << surfaceProps[2] << endl;
	if (hasTex)
		texture.dump(ind, detailed);

	ind = ind.substr(0, ind.size()-2);
	cout << ind << "}\n";
}

Material::Material(void)
{
	hasMatFx = false;
	matFx = 0;
	hasRightToRender = false;
	hasReflectionMat = false;
	hasSpecularMat = false;
}

Material::Material(const Material& orig)
: flags(orig.flags), unknown(orig.unknown),
  hasTex(orig.hasTex), texture(orig.texture),
  hasRightToRender(orig.hasRightToRender),
  rightToRenderVal1(orig.rightToRenderVal1),
  rightToRenderVal2(orig.rightToRenderVal2),
  hasMatFx(orig.hasMatFx), hasReflectionMat(orig.hasReflectionMat),
  reflectionIntensity(orig.reflectionIntensity),
  hasSpecularMat(orig.hasSpecularMat), specularLevel(orig.specularLevel),
  specularName(orig.specularName)
{
	if (orig.matFx)
		matFx = new MatFx(*orig.matFx);
	else
		matFx = 0;

	for (uint32 i = 0; i < 4; i++)
		color[i] = orig.color[i];
	for (uint32 i = 0; i < 3; i++)
		surfaceProps[i] = orig.surfaceProps[i];
	for (uint32 i = 0; i < 4; i++)
		reflectionChannelAmount[i] = orig.reflectionChannelAmount[i];
}

Material &Material::operator=(const Material &that)
{
	if (this != &that) {
		flags = that.flags;
		for (uint32 i = 0; i < 4; i++)
			color[i] = that.color[i];
		unknown = that.unknown;
		hasTex = that.hasTex;
		for (uint32 i = 0; i < 3; i++)
			surfaceProps[i] = that.surfaceProps[i];

		texture = that.texture;

		hasRightToRender = that.hasRightToRender;
		rightToRenderVal1 = that.rightToRenderVal1;
		rightToRenderVal2 = that.rightToRenderVal2;

		hasMatFx = that.hasMatFx;
		delete matFx;
		matFx = 0;
		if (that.matFx)
			matFx = new MatFx(*that.matFx);

		hasReflectionMat = that.hasReflectionMat;
		for (uint32 i = 0; i < 4; i++)
			reflectionChannelAmount[i] =
					that.reflectionChannelAmount[i];
		reflectionIntensity = that.reflectionIntensity;

		hasSpecularMat = that.hasSpecularMat;
		specularLevel = that.specularLevel;
		specularName = that.specularName;
	}
	return *this;
}

Material::~Material(void)
{
	delete matFx;
	matFx = 0;
}


MatFx::MatFx(void)
{
	hasTex1 = false;
	hasTex2 = false;
	hasDualPassMap = false;
}


/*
 * Texture
 */

void Texture::read(ifstream &rw)
{
	HeaderInfo header;

	READ_HEADER(CHUNK_TEXTURE);

	READ_HEADER(CHUNK_STRUCT);
	filterFlags = readUInt16(rw);
	rw.seekg(2, ios::cur);

	READ_HEADER(CHUNK_STRING);
	char *buffer = new char[header.length+1];
	rw.read(buffer, header.length);
	buffer[header.length] = '\0';
	name = buffer;
	delete[] buffer;

	READ_HEADER(CHUNK_STRING);
	buffer = new char[header.length+1];
	rw.read(buffer, header.length);
	buffer[header.length] = '\0';
	maskName = buffer;

	readExtension(rw);
}

void Texture::readExtension(ifstream &rw)
{
	HeaderInfo header;

	READ_HEADER(CHUNK_EXTENSION);

	uint32 end = rw.tellg();
	end += header.length;

	while (rw.tellg() < end) {
		header.read(rw);
		switch (header.type) {
		case CHUNK_SKYMIPMAP:
			hasSkyMipmap = true;
			rw.seekg(header.length, ios::cur);
			break;
		default:
			rw.seekg(header.length, ios::cur);
			break;
		}
	}
}

void Texture::dump(std::string ind, bool detailed)
{
	cout << ind << "Texture {\n";
	ind += "  ";

	cout << ind << "filterFlags: " << hex << filterFlags << dec << endl;
	cout << ind << "name: " << name << endl;
	cout << ind << "maskName: " << maskName << endl;

	ind = ind.substr(0, ind.size()-2);
	cout << ind << "}\n";
}

Texture::Texture(void) { hasSkyMipmap = false; }

}

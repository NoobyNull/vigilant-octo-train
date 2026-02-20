#include "default_materials.h"

namespace dw {

namespace {

MaterialRecord makeHardwood(const std::string& name,
                             float janka,
                             float feed,
                             float spindle,
                             float doc,
                             float cost) {
    MaterialRecord r;
    r.name = name;
    r.category = MaterialCategory::Hardwood;
    r.jankaHardness = janka;
    r.feedRate = feed;
    r.spindleSpeed = spindle;
    r.depthOfCut = doc;
    r.costPerBoardFoot = cost;
    r.grainDirectionDeg = 0.0f;
    return r;
}

MaterialRecord makeSoftwood(const std::string& name,
                             float janka,
                             float feed,
                             float spindle,
                             float doc,
                             float cost) {
    MaterialRecord r;
    r.name = name;
    r.category = MaterialCategory::Softwood;
    r.jankaHardness = janka;
    r.feedRate = feed;
    r.spindleSpeed = spindle;
    r.depthOfCut = doc;
    r.costPerBoardFoot = cost;
    r.grainDirectionDeg = 0.0f;
    return r;
}

MaterialRecord makeDomestic(const std::string& name,
                             float janka,
                             float feed,
                             float spindle,
                             float doc,
                             float cost) {
    MaterialRecord r;
    r.name = name;
    r.category = MaterialCategory::Domestic;
    r.jankaHardness = janka;
    r.feedRate = feed;
    r.spindleSpeed = spindle;
    r.depthOfCut = doc;
    r.costPerBoardFoot = cost;
    r.grainDirectionDeg = 0.0f;
    return r;
}

MaterialRecord makeComposite(const std::string& name,
                              float feed,
                              float spindle,
                              float doc,
                              float cost) {
    MaterialRecord r;
    r.name = name;
    r.category = MaterialCategory::Composite;
    r.jankaHardness = 0.0f; // N/A for composites/metals/plastics
    r.feedRate = feed;
    r.spindleSpeed = spindle;
    r.depthOfCut = doc;
    r.costPerBoardFoot = cost;
    r.grainDirectionDeg = 0.0f;
    return r;
}

} // namespace

std::vector<MaterialRecord> getDefaultMaterials() {
    std::vector<MaterialRecord> materials;
    materials.reserve(32);

    // --- Hardwoods (8) ---
    // Janka ratings from The Wood Database; CNC defaults conservative for hobby machines
    materials.push_back(makeHardwood("Red Oak",      1290.0f, 80.0f,  18000.0f, 0.125f, 4.50f));
    materials.push_back(makeHardwood("White Oak",    1360.0f, 75.0f,  18000.0f, 0.100f, 5.00f));
    materials.push_back(makeHardwood("Hard Maple",   1450.0f, 70.0f,  18000.0f, 0.100f, 5.50f));
    materials.push_back(makeHardwood("Cherry",        995.0f, 85.0f,  18000.0f, 0.125f, 6.00f));
    materials.push_back(makeHardwood("Black Walnut", 1010.0f, 85.0f,  18000.0f, 0.125f, 8.00f));
    materials.push_back(makeHardwood("White Ash",    1320.0f, 75.0f,  18000.0f, 0.100f, 4.00f));
    materials.push_back(makeHardwood("Yellow Birch", 1260.0f, 80.0f,  18000.0f, 0.125f, 4.50f));
    materials.push_back(makeHardwood("Hickory",      1820.0f, 60.0f,  18000.0f, 0.075f, 5.00f));

    // --- Softwoods (7) ---
    materials.push_back(makeSoftwood("White Pine",         380.0f, 150.0f, 16000.0f, 0.250f, 2.00f));
    materials.push_back(makeSoftwood("Yellow Pine",        870.0f, 100.0f, 18000.0f, 0.150f, 2.50f));
    materials.push_back(makeSoftwood("Douglas Fir",        660.0f, 110.0f, 16000.0f, 0.200f, 3.00f));
    materials.push_back(makeSoftwood("Western Red Cedar",  350.0f, 150.0f, 14000.0f, 0.250f, 4.00f));
    materials.push_back(makeSoftwood("Spruce",             490.0f, 130.0f, 16000.0f, 0.200f, 2.00f));
    materials.push_back(makeSoftwood("Eastern Hemlock",    500.0f, 120.0f, 16000.0f, 0.200f, 2.50f));
    materials.push_back(makeSoftwood("Redwood",            420.0f, 140.0f, 14000.0f, 0.250f, 6.00f));

    // --- Domestic (7) ---
    // Common North American species not strictly hardwood or softwood
    materials.push_back(makeDomestic("Soft Maple",   950.0f, 90.0f,  18000.0f, 0.150f, 3.50f));
    materials.push_back(makeDomestic("Poplar",       540.0f, 120.0f, 16000.0f, 0.200f, 2.50f));
    materials.push_back(makeDomestic("Alder",        590.0f, 110.0f, 16000.0f, 0.200f, 3.00f));
    materials.push_back(makeDomestic("Beech",       1300.0f, 75.0f,  18000.0f, 0.100f, 4.00f));
    materials.push_back(makeDomestic("Basswood",     410.0f, 140.0f, 14000.0f, 0.250f, 3.00f));
    materials.push_back(makeDomestic("Butternut",    490.0f, 120.0f, 16000.0f, 0.200f, 5.00f));
    materials.push_back(makeDomestic("Cottonwood",   430.0f, 130.0f, 16000.0f, 0.200f, 2.00f));

    // --- Composites (10) ---
    // Engineered panels, non-ferrous metals, plastics, foams — Janka N/A (0)
    materials.push_back(makeComposite("MDF",                  100.0f, 18000.0f, 0.125f, 1.50f));
    materials.push_back(makeComposite("HDF",                   90.0f, 18000.0f, 0.100f, 2.00f));
    materials.push_back(makeComposite("Baltic Birch Plywood",  90.0f, 18000.0f, 0.100f, 3.00f));
    materials.push_back(makeComposite("Hardwood Plywood",      90.0f, 18000.0f, 0.100f, 2.50f));
    materials.push_back(makeComposite("Particle Board",       110.0f, 16000.0f, 0.150f, 1.00f));
    materials.push_back(makeComposite("Aluminum (6061)",       30.0f, 10000.0f, 0.050f, 0.00f));
    materials.push_back(makeComposite("Brass",                 25.0f,  8000.0f, 0.040f, 0.00f));
    materials.push_back(makeComposite("HDPE",                 120.0f, 16000.0f, 0.200f, 0.00f));
    materials.push_back(makeComposite("Acrylic",               60.0f, 12000.0f, 0.100f, 0.00f));
    materials.push_back(makeComposite("Rigid Foam (PVC)",     150.0f, 14000.0f, 0.300f, 0.00f));

    return materials;
}

} // namespace dw

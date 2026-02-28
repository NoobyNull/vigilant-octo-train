# Phase 05-05: The Descriptor Integration Plan

**Status**: Steps 1-5 Completed ✅ | Steps 6-9 Planning

## Summary
Integrating Gemini Vision AI to automatically classify 3D model thumbnails and assign hierarchical categories.

## Completed Steps

### Step 1: HTTP Utilities Extraction ✅
- Created `src/core/utils/gemini_http.h` and `.cpp`
- Extracted shared HTTP primitives: `curlPost()`, `base64Encode/Decode()`
- Uses `dw::gemini` namespace
- Avoids duplication between GeminiMaterialService and GeminiDescriptorService

### Step 2: GeminiDescriptorService ✅
- Created `src/core/materials/gemini_descriptor_service.h` and `.cpp`
- Rich `DescriptorResult` struct with:
  - `title`: Short AI-generated name
  - `description`: Detailed classification
  - `hoverNarrative`: One-line UI tooltip
  - `keywords`: 3-5 searchable terms
  - `associations`: Related brands/logos/artists
  - `categories`: Hierarchical chain (broad → specific)
- Service methods:
  - `describe()`: Load TGA, convert to PNG, send to Gemini Vision API, parse response
  - `tgaToPng()`: Image conversion using stb_image
  - `fetchClassification()`: Gemini API communication
  - `parseClassification()`: JSON response parsing

### Step 3: Schema Migration v8 ✅
- Updated `CURRENT_VERSION` from 7 to 8
- Added migration logic for three new TEXT columns:
  - `descriptor_title`
  - `descriptor_description`
  - `descriptor_hover`
- Applied to models table

### Step 4: ModelRepository Updates ✅
- Extended `ModelRecord` with descriptor fields:
  - `descriptorTitle`, `descriptorDescription`, `descriptorHover`
- Implemented `updateDescriptor()` method (INSERT/UPDATE)
- Implemented `findCategoryByNameAndParent()` for category lookup
- Updated `rowToModel()` to read descriptor fields (columns 26-28)

### Step 5: LibraryManager Category Chain Resolution ✅
- Implemented `updateDescriptor()` method
  - Delegates to ModelRepository
  - Updates model's AI classification metadata
- Implemented `resolveAndAssignCategories()` method
  - Takes category chain array: `["Character", "Fantasy", "Dragon"]`
  - Creates missing categories with parent-child relationships
  - Auto-creates intermediate categories if needed
  - Assigns leaf category to model
  - Example hierarchy:
    - Category: "Character" (root, parent_id=NULL)
    - Category: "Fantasy" (parent_id=Character.id)
    - Category: "Dragon" (parent_id=Fantasy.id)
    - model_categories: (modelId, Dragon.id)

## Remaining Steps

### Step 6: Wire Auto-Describe on Import 🔄
**Objective**: Integrate descriptor service into import pipeline

**Tasks**:
1. Add GeminiDescriptorService instance to Application
2. Modify FileIOManager::processCompletedImports() to:
   - Queue descriptor task after thumbnail generation
   - Fetch Gemini API key from Config
   - Call GeminiDescriptorService::describe() on worker thread
   - On completion callback:
     - Call LibraryManager::updateDescriptor() to save results
     - Call LibraryManager::resolveAndAssignCategories() to assign categories
3. Add optional flag to disable auto-describe in config

**Key Architecture Notes**:
- descriptor service call should be non-blocking (worker thread)
- Must not block UI during Gemini API latency
- Should gracefully handle API failures (warn, don't fail import)
- Gemini API key may be empty (skip descriptor in that case)

### Step 7: Manual "Describe with AI" Context Menu 🔄
**Objective**: Allow users to manually trigger description after import

**Tasks**:
1. Add context menu entry to LibraryPanel's model items
2. Show loading spinner during API call
3. Dialog to show results with:
   - AI-generated title and description
   - Option to accept/edit/discard
   - Preview of assigned categories
4. On accept: save via LibraryManager methods

### Step 8: Display Descriptor Metadata in UI 🔄
**Objective**: Show AI classification in properties panel and library

**Tasks**:
1. Properties Panel: Add "AI Classification" section showing:
   - Title with hover tooltip (descriptor_hover)
   - Full description (descriptor_description)
   - Keywords as tags
   - Categories as breadcrumb/chain
   - Associations as text
2. Library Panel: Show descriptor_title as tooltip on hover
3. Search: Include descriptor fields in FTS index

### Step 9: Build and Verify 🔄
**Objective**: Ensure all changes compile and function correctly

**Tasks**:
1. Full build validation
2. Test import pipeline with auto-describe
3. Verify schema migration (v7 → v8)
4. Test category chain creation and assignment
5. Manual descriptor testing with test model

## Technical Notes

### Gemini Vision API Request Format
```json
{
  "contents": [{
    "parts": [
      {
        "text": "Classify this 3D model thumbnail..."
      },
      {
        "inlineData": {
          "mimeType": "image/jpeg",
          "data": "base64-encoded-image"
        }
      }
    ]
  }]
}
```

### Expected Response Format
```json
{
  "title": "Dragon Figurine",
  "description": "Detailed fantasy classification...",
  "hoverNarrative": "A stylized fantasy dragon.",
  "keywords": ["dragon", "fantasy", "figurine"],
  "associations": ["Blizzard Entertainment", "D&D"],
  "categories": ["Character", "Fantasy", "Dragon"]
}
```

### Category Chain Flow
```
Gemini Response Categories
        ↓
LibraryManager::resolveAndAssignCategories()
        ↓
Create/Find each category level
        ↓
Assign leaf category to model (model_categories junction)
        ↓
Graph layer (dual-write if available)
```

## Files Modified

**Created**:
- `src/core/utils/gemini_http.h/cpp` - Shared HTTP utilities
- `src/core/materials/gemini_descriptor_service.h/cpp` - Descriptor service

**Modified**:
- `src/core/database/schema.h/cpp` - Schema v8 migration
- `src/core/database/model_repository.h/cpp` - Descriptor fields and methods
- `src/core/library/library_manager.h/cpp` - Category chain resolution
- `src/CMakeLists.txt` - Added descriptor service build target

**Next to modify**:
- `src/managers/file_io_manager.h/cpp` - Wire async descriptor
- `src/ui/panels/library_panel.h/cpp` - Context menu and tooltips
- `src/ui/panels/properties_panel.h/cpp` - Display AI classification

## Testing Checklist

- [ ] Step 6: Models describe after import (check logs)
- [ ] Step 6: Gemini API failures don't break import
- [ ] Step 7: Context menu appears on right-click
- [ ] Step 7: Manual describe triggers and updates model
- [ ] Step 8: Properties panel shows descriptor metadata
- [ ] Step 8: Library panel shows descriptor title on hover
- [ ] Step 9: Full build succeeds
- [ ] Step 9: Schema migration v7→v8 works
- [ ] Step 9: Verify database columns added correctly
- [ ] Step 9: Category chain creation works for multi-level hierarchies

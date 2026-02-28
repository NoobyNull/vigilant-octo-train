---
area: project
priority: medium
slug: document-attachment-system
created: 2026-02-22
---

# Document/Attachment System

Associate arbitrary files (PDF, XHTML, images, notes) with a project. Requires:

- `project_attachments` table (project_id, filename, mime_type, blob_hash, sort_order)
- Store attachments in CAS blob store (reuse StorageManager)
- UI: drag-and-drop files onto Project Panel to attach
- Export: include attachments in .dwproj archive under `attachments/` directory
- Import: restore attachments from archive

Extension point: Phase 6 schema should include a `project_attachments` table placeholder or leave space in the manifest for an `attachments` section.

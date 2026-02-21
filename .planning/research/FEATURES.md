# Feature Landscape

**Domain:** CNC Woodworking Workshop Management Software
**Researched:** 2026-02-08
**Confidence:** MEDIUM

## Table Stakes

Features users expect. Missing = product feels incomplete.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Cut list generation with optimization | Core woodworking workflow - material costs are significant | Medium | Already have 2D bin-packing + guillotine. Need grain direction, edge banding tracking, 1D optimization for linear materials |
| Job costing & pricing | Professional shops need profitability tracking per job | Medium | Database schema exists but UI incomplete. Need material + labor cost tracking against estimates |
| PDF quote/estimate generation | Standard professional business practice | Low | Must include itemized costs, terms/conditions, branding customization |
| Tool database with feeds/speeds | Critical for CNC operations - prevents tool breakage and poor cuts | Medium | Need database of router bits with RPM, feed rate, chip load calculations for different materials |
| Wood species database | Material selection drives grain, cost, availability decisions | Low | Properties: density, grain direction, cost/board-foot, common uses |
| Project/job tracking | Workshop organization fundamental | Low | Already have project management - extend with job folders, status tracking, scheduling |
| Customer management | Business record keeping requirement | Low | Customer details, contact info, job history, quote tracking |
| G-code visualization | Verifying toolpaths before running prevents disasters | Medium | Already have 2D path visualization. Need 3D simulation with stock removal |
| Material inventory tracking | Knowing what's in stock prevents ordering delays | Medium | Track sheet goods (plywood, MDF) and dimensional lumber by species, thickness, quantity |
| Time tracking | Accurate labor costing requires actual time data | Low | Track actual time per job/task vs. estimates for better future quotes |

## Differentiators

Features that set product apart. Not expected, but valued.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| CNC machine control integration | Control machine from same software as project planning | High | REST/WebSocket API to controllers like ncSender, Buildbotics. Real-time status monitoring |
| Advanced G-code simulation with collision detection | Catch crashes before they happen - saves tools, materials, machines | High | Full 3D toolpath simulation showing stock removal, tool/fixture collision detection, travel limit checking |
| Real-time job costing with profit tracking | Know profitability during production, not after | Medium | Compare actual costs vs. estimates in real-time as materials used and time logged |
| Automated tool life management | Prevent tool wear defects, optimize replacement schedules | Medium | Track router bit usage hours/passes, alert for replacement based on wear data |
| Shop floor production monitoring | Visibility into what's running, queue status, bottlenecks | Medium | OEE (Overall Equipment Effectiveness) metrics, machine status dashboards |
| Advanced cut list optimization with grain matching | Minimize waste while respecting wood grain aesthetics | High | Consider grain direction, book matching, sequential matching in optimization |
| Edge banding automation | Track and plan edge banding as part of cut list | Medium | Specify which edges need banding by species/color, auto-calculate linear footage |
| 3D design import with auto-cut-list | From model to cut list without manual entry | High | Extract dimensions, quantities, grain orientation from 3D models (STL/OBJ/3MF already supported) |
| Watch folder auto-import | Drop files, automatically process and catalog | Low | Already in missing features list - reduces manual import steps |
| Nested project workspaces | Manage sub-assemblies and component libraries | Medium | Cabinets have drawers, doors, shelves - hierarchical project structure |
| Material/PBR rendering | Professional client presentations | Medium | Already in missing features - realistic wood grain visualization for quotes |

## Anti-Features

Features to explicitly NOT build.

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|-------------------|
| Full CAD/CAM design tools | Already exists (Fusion 360, VCarve, Aspire) - can't compete | Focus on importing and managing designs, not creating them |
| Custom CNC controller firmware | Hardware-specific, high complexity, liability | Integrate with existing controllers via standard APIs (REST/WebSocket) |
| Accounting software replacement | QuickBooks/Xero exist, feature bloat | Export invoices/estimates for import to accounting software |
| Real-time collaboration | Woodworking shops are typically single-user or small team, local network | Focus on solid single-user experience with project export/import |
| Cloud-required architecture | Many shops have unreliable internet or prefer local control | Desktop-first with optional cloud backup/sync |
| Generic ERP features | Payroll, HR, multi-location inventory across warehouses | Stay focused on CNC woodworking workshop operations |
| Machine learning optimizations | Limited data in small shops, complexity doesn't justify gains | Use proven algorithms (bin-packing, guillotine) that are deterministic |
| Mobile app for production | Shop floor is dusty, touch screens problematic | Focus on desktop for planning, simple tablet view for time tracking only |

## Feature Dependencies

```
Customer Management
    └──requires──> Job/Project Tracking
                      └──requires──> Quote Generation (PDF)
                                        └──requires──> Job Costing/Pricing
                                                          └──requires──> Material Database
                                                          └──requires──> Tool Database

G-code 3D Simulation
    └──requires──> G-code Parser (✓ already have)
    └──requires──> Tool Database (for tool geometry)

Cut List with Grain/Edge Banding
    └──requires──> Wood Species Database
    └──requires──> Material Inventory
    └──enhances──> 2D Cut Optimizer (✓ already have)

CNC Machine Control
    └──requires──> G-code Parser (✓ already have)
    └──conflicts──> Multiple controller types in one release (pick one controller API first)

Real-time Job Costing
    └──requires──> Job Costing/Pricing (basic)
    └──requires──> Time Tracking
    └──requires──> Material Inventory

Tool Life Management
    └──requires──> Tool Database
    └──requires──> CNC Machine Control (to track actual usage)

Production Monitoring
    └──requires──> CNC Machine Control
    └──requires──> Time Tracking
```

### Dependency Notes

- **Customer → Job → Quote → Costing pipeline:** Standard business workflow - each step depends on previous
- **Material/Tool databases are foundational:** Multiple features reference these - build early
- **G-code features build incrementally:** 2D visualization (✓ have) → 3D simulation → collision detection
- **CNC machine control is a multiplier:** Enables monitoring, tool tracking, but complex - defer until core features solid
- **Advanced cut list features enhance existing optimizer:** Already have bin-packing, add grain/edge banding awareness
- **Real-time costing requires basic costing first:** Don't jump to real-time without solid estimate foundation

## MVP Recommendation

### Already Have (Current State)
Digital Workshop already includes:
- 3D model library with STL/OBJ/3MF import, thumbnails, tags, search
- 3D viewport with orbit camera
- G-code parser with 2D path visualization and timing
- 2D cut optimizer (bin-pack + guillotine)
- Project management with archives
- Cost estimation database schema (UI incomplete)
- Cross-platform (Linux/Windows)

### Launch Next Milestone With (Priority Order)

1. **Wood Species Database** (P1 - foundational)
   - Properties: density, grain characteristics, cost/board-foot, common uses
   - Why: Referenced by cut list, inventory, costing

2. **Tool Database with Feeds/Speeds** (P1 - foundational)
   - Router bit specs: diameter, flutes, RPM ranges, feed rates, chip loads
   - Material-specific cutting parameters
   - Why: Critical for safe/effective CNC operation, referenced by simulation and machine control

3. **Complete Cost Estimation UI** (P1 - table stakes)
   - Finish existing database schema implementation
   - Material + labor + overhead calculation
   - Why: Database exists, just needs UI - quick win

4. **Customer Management** (P1 - table stakes)
   - Customer records, contact info, job history
   - Quote tracking and status
   - Why: Necessary for professional business operations

5. **PDF Quote/Estimate Generation** (P1 - table stakes)
   - Professional templates with itemized costs
   - Terms, conditions, expiration dates
   - Customizable branding/layout
   - Why: Universal business requirement

6. **Enhanced Cut List** (P2 - leverages existing optimizer)
   - Grain direction tracking and optimization
   - Edge banding specification (species, which edges)
   - Linear (1D) optimization for dimensional lumber
   - Why: Builds on existing 2D optimizer, significant material cost savings

7. **Material Inventory Tracking** (P2 - enables better costing)
   - Sheet goods tracking (species, thickness, quantity)
   - Dimensional lumber tracking
   - Why: Supports accurate job costing and purchasing decisions

### Add After Core Complete (v1.x)

- **Job Costing with Time Tracking** - Track actual vs. estimated costs
- **Advanced G-code 3D Simulation** - Stock removal visualization, travel verification
- **Watch Folder Auto-Import** - Workflow automation for model ingestion
- **Material/PBR Rendering** - Professional client presentations
- **Undo/Redo System** - UX improvement across application
- **Advanced Search** - Find models, jobs, customers faster

### Future Consideration (v2+)

- **CNC Machine Control Integration** - Requires controller API integration (REST/WebSocket)
- **Collision Detection in G-code Sim** - Advanced safety feature
- **Real-time Job Costing** - Profit tracking during production
- **Tool Life Management** - Automated replacement scheduling
- **Shop Floor Production Monitoring** - OEE metrics, dashboards
- **Advanced Grain Matching** - Book matching, sequential matching in optimization
- **Nested Project Workspaces** - Component library, sub-assemblies

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority | Notes |
|---------|------------|---------------------|----------|-------|
| Wood Species Database | HIGH | LOW | P1 | Foundational data structure |
| Tool Database + Feeds/Speeds | HIGH | MEDIUM | P1 | Complex calculations but well-documented |
| Complete Cost Estimation UI | HIGH | LOW | P1 | Schema exists, just needs UI |
| Customer Management | HIGH | LOW | P1 | Standard CRUD operations |
| PDF Quote Generation | HIGH | LOW | P1 | Libraries exist (Qt PDF or reportlab) |
| Enhanced Cut List (grain/edge) | HIGH | MEDIUM | P2 | Extends existing optimizer |
| Material Inventory | MEDIUM | MEDIUM | P2 | Database + UI, stock tracking logic |
| Time Tracking | MEDIUM | LOW | P2 | Simple start/stop timers per job |
| Job Costing (basic) | HIGH | MEDIUM | P2 | Combine estimates + actuals |
| 3D G-code Simulation | MEDIUM | HIGH | P2 | 3D rendering + toolpath interpretation |
| Watch Folder Auto-Import | LOW | LOW | P2 | File system monitoring |
| Undo/Redo | MEDIUM | HIGH | P2 | Application-wide command pattern |
| PBR Material Rendering | LOW | MEDIUM | P3 | Graphics programming |
| CNC Machine Control | MEDIUM | HIGH | P3 | Controller API integration, testing |
| Collision Detection | MEDIUM | HIGH | P3 | Geometric intersection algorithms |
| Real-time Job Costing | MEDIUM | MEDIUM | P3 | Requires basic costing + time tracking first |
| Tool Life Management | LOW | HIGH | P3 | Requires machine control integration |
| Production Monitoring (OEE) | LOW | HIGH | P3 | Requires machine control + analytics |
| Advanced Grain Matching | LOW | HIGH | P3 | Complex optimization constraints |

**Priority key:**
- P1: Must have for professional credibility - table stakes
- P2: Should have, differentiates from basic tools
- P3: Nice to have, advanced features for power users

## Competitor Feature Analysis

| Feature | Microvellum (High-end) | CabinetShop Maestro (Mid-tier) | VCarve/Aspire (CAM-focused) | Digital Workshop Approach |
|---------|------------------------|--------------------------------|-----------------------------|---------------------------|
| CAD/CAM Design | Full 3D cabinet design | Limited design | Advanced 2.5D/3D carving | Import only - don't compete |
| Cut List Optimization | Yes, with grain/edge | Yes, basic | Yes, nesting | Enhance existing optimizer with grain/edge |
| Job Costing | Advanced real-time | Basic estimate vs actual | No - CAM only | Start basic, evolve to real-time |
| Customer/Quote Management | Full ERP integration | Yes, integrated | No | Integrated but focused |
| G-code Generation | Yes, machine-specific | Limited | Yes, sophisticated | Import and visualize, not generate |
| Machine Control | Yes, proprietary | No | Yes, via partners | Integrate with existing controllers (REST API) |
| Tool Database | Comprehensive | Basic | Extensive for carving | Focus on router bits, feeds/speeds |
| Material Database | Yes, with pricing | Yes | Limited | Wood species focus with properties |
| 3D Simulation | Full collision detection | Basic visualization | Advanced toolpath preview | Progressive: 2D→3D→collision |
| Inventory Tracking | Multi-location warehouse | Shop-level | No | Single shop focus |
| Time Tracking | Shop floor tablets | Manual entry | No | Simple start/stop timers |
| PDF Export | Highly customizable | Standard templates | Basic reports | Professional templates, customizable |
| Price Point | $15k-50k+ | $2k-5k | $350-2k | Open-source / Free |

**Our Competitive Position:**
- **Avoid:** CAD/CAM design tools (Fusion 360, VCarve dominate)
- **Match:** Core business features (customer, quotes, costing, inventory)
- **Differentiate:** Integration across workflow (import → optimize → cost → quote → track), open-source, cross-platform, no vendor lock-in

## Sources

**CNC Shop Management & Features:**
- [5 Best OEE Monitoring Software Tools for CNC Shops (2026)](https://www.fabrico.io/blog/best-oee-software-cnc-machine-shops/) - MEDIUM confidence
- [Microvellum Woodworking Manufacturing Software](https://www.microvellum.com/solutions/manufacturing) - MEDIUM confidence
- [INNERGY Software for Woodworkers](https://www.innergy.com/) - MEDIUM confidence

**Software Requirements & Workflows:**
- [The Best CNC Router Software - Mastercam](https://www.mastercam.com/news/blog/the-best-cnc-router-software/) - MEDIUM confidence
- [BESTIN GROUP CNC Router Software Guide](https://www.bestingroup.com/cnc-router-software/) - MEDIUM confidence

**Tool & Material Databases:**
- [CNC Feeds and Speeds Calculator 2026](https://www.cncoptimization.com/calculators/feeds-speeds/) - MEDIUM confidence
- [Popular Woodworking: Feeds & Speeds for CNC Routers](https://www.popularwoodworking.com/techniques/feeds-speeds-for-cnc-routers/) - MEDIUM confidence
- [Wood Database](https://www.wood-database.com/wood-articles/wood-identification-guide/) - MEDIUM confidence

**Customer Management & Quoting:**
- [Orderry Carpentry Software](https://orderry.com/carpenter-business-software/) - MEDIUM confidence
- [CabinetShop Maestro](https://www.cabinetshopsoftware.com/) - MEDIUM confidence
- [Microvellum Estimating Solutions](https://www.microvellum.com/solutions/estimating) - MEDIUM confidence
- [Buildxact Carpentry Estimating Software](https://www.buildxact.com/us/carpentry-estimating-software/) - MEDIUM confidence

**Job Costing & Inventory:**
- [CabinetShop Software Features](https://www.cabinetshopsoftware.com/features) - MEDIUM confidence
- [Pro100 Cabinet Job Costing Software](https://www.pro100usa.com/support/how-it-works/job-costing-software) - MEDIUM confidence
- [Digit Furniture & Woodworking ERP](https://www.digit-software.com/industries/furniture-woodworking) - MEDIUM confidence
- [Epicor LumberTrack](https://www.epicor.com/en-us/products/enterprise-resource-planning-erp/lumbertrack/) - MEDIUM confidence

**Machine Monitoring & Tool Tracking:**
- [TMAC Tool Monitoring System](https://www.caroneng.com/products/tmac/) - MEDIUM confidence
- [Tool Wear Detection and Maintenance in CNC Machining](https://jlccnc.com/blog/tool-wear-detection-maintenance) - MEDIUM confidence
- [MachineMetrics Tool Monitoring](https://www.machinemetrics.com/blog/tool-monitoring) - MEDIUM confidence

**CNC Control APIs:**
- [GitHub: cncserver - RESTful API for CNC devices](https://github.com/techninja/cncserver) - MEDIUM confidence
- [Buildbotics CNC Controller API](https://buildbotics.com/introduction-to-the-buildbotics-cnc-controller-api/) - MEDIUM confidence
- [WebControl - Browser-based CNC Control](https://webcontrolcnc.github.io/WebControl/) - MEDIUM confidence

**G-code Simulation:**
- [G-Code Simulators: The Ultimate Guide - Vericut](https://vericut.com/resources/guides/g-code-simulator) - MEDIUM confidence
- [5 Best G-Code Simulators for Machining](https://robodk.com/blog/g-code-simulators-machining/) - MEDIUM confidence
- [CAMotics](https://camotics.org/) - MEDIUM confidence
- [Vericut CNC Machine Simulation](https://vericut.com/products/cnc-machine-simulation) - MEDIUM confidence
- [Predator Virtual CNC Software](https://www.predator-software.com/predator_virtual_cnc_software.htm) - MEDIUM confidence

**Edge Banding & Grain:**
- [SketchList3D Edge Banding in Cabinet Design](https://sketchlist.com/blog/edge-banding-in-cabinet-design-software-projects/) - MEDIUM confidence
- [SWOOD CAD for Woodworking Design](https://store.trimech.com/blog/swood-cad-game-changer-for-woodworking-design-manufacturing) - MEDIUM confidence

**Confidence Notes:**
- All sources are WebSearch results verified across multiple sources
- No Context7 or official documentation available for domain-specific woodworking software
- Confidence level MEDIUM based on multiple sources agreeing on feature sets
- Industry practices confirmed across commercial products (Microvellum, CabinetShop, VCarve)
- Some features (tool databases, feeds/speeds) have authoritative technical sources

---
*Feature research for: CNC Woodworking Workshop Management Software*
*Researched: 2026-02-08*

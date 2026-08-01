# Manuals

Per-model programming/interface manuals and datasheets live here, one folder per
model: `manuals/<Model>/`. **These PDFs are development inputs and are never
shipped** (the packaging leak-guard enforces this).

Until a model's manual is present, that model's SCPI and numeric limits are
seeded from datasheet headline specs and standard per-vendor dialects, tagged
`TODO(manual)` in `Common/S_ScopeLimits.h` and in the plugin sources. Each
`<Model>/README.md` lists exactly what to obtain and what to verify.

## Sourcing recipe (from the build prompt, section 1b)

For every model obtain **two** documents:
1. the **Programming / Interface manual** (the SCPI reference - the single
   source of truth for every command, range and response format), and
2. the **datasheet / user manual** (the per-model spec table source).

- **Digi-Key** (`digikey.com`): search the exact model -> product page ->
  *Documents & Media* -> download the Datasheet and Programming/Reference Manual.
- **Mouser** (`mouser.com`): search the exact model -> *Product Documents /
  Datasheets & Files* -> download the Datasheet and Manual.
- **Manufacturer fallback** (common for bench instruments): Tektronix `tek.com`,
  Keysight `keysight.com`, Rohde & Schwarz `rohde-schwarz.com`, Teledyne LeCroy
  `teledynelecroy.com`.

If a document cannot be retrieved, leave the model against SimScope + MockVisa
with its `TODO(manual)` markers rather than inventing SCPI.

## Status

| Model | Vendor | Programming manual | Datasheet specs |
|-------|--------|--------------------|-----------------|
| MDO34 | Tektronix | TODO(manual) | verified (headline) |
| RTM3004 | Rohde & Schwarz | TODO(manual) | verified (headline) |
| DSO7104B | Keysight (Agilent) | TODO(manual) | TODO(manual) |
| DSOS204A | Keysight | TODO(manual) | TODO(manual) |
| DSOX2012A | Keysight (Agilent) | TODO(manual) | TODO(manual) |
| MSO6054A | Agilent (Keysight) | TODO(manual) | TODO(manual) |
| RTO2064 | Rohde & Schwarz | TODO(manual) | TODO(manual) |
| TDS1012B | Tektronix | TODO(manual) | TODO(manual) |
| TDS2024C | Tektronix | TODO(manual) | TODO(manual) |
| WaveSurfer42Xs | Teledyne LeCroy | TODO(manual) | TODO(manual) |

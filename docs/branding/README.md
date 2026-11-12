# ChaoN-USB branding

ChaoN expands to **Chaotic Neutral USB**. Its compact project identity is the **emerald-and-gold natural-20 d20**, and its approved illustrated mascot is **Sara, a charismatic pixel-art rogue-bard with a weasel familiar**.

The [visual direction](../branding.md) describes the color and typography system. The SVG marks are deliberately separate from the original illustrated d20 and mascot.

## Asset inventory

These three **approved original PNG binaries are committed in this directory**. Dimensions, pixel modes, byte lengths and SHA-256 digests below were independently inventoried at review head `ee31df2661d5d3dc664b4dea18b63fc1a61f82fe`; re-verify before claiming identity after any future changes.

| File | Size | Mode | Bytes | SHA-256 |
| --- | --- | --- | ---: | --- |
| [Emerald ornate d20](emerald_gold_filigree_d20.png) | 1254 × 1254 | RGBA | 2,112,305 | `56171f49cbdd8419108190bb0edf4b4f78bc50e204312fd3d839e0eddba3312d` |
| [Sara rogue-bard mascot](pixel_catgirl_bard_with_ferret_familiar.png) | 1254 × 1254 | RGBA | 1,767,920 | `3a4245b69305c2f52fb2ebb8d0cf507b2de2eae3a1159114f481915cc217a918` |
| [Social preview](social-open-graph-1200x630.png) | 1200 × 630 | RGB | 876,287 | `d2bdbc5e14eade216e4ccdc5fabedadfc49df7aa6b058da8d48cbd280e642b5d` |

The mascot source filename retains **ferret** for reproducibility, but the established character familiar is a **weasel**. Do not silently rename or regenerate approved binary assets.

### Vector assets

- [`assets/chaon-logo.svg`](../../assets/chaon-logo.svg): restrained horizontal README header/wordmark with triangular d20 face and outlined number.
- [`assets/chaon-d20.svg`](../../assets/chaon-d20.svg): simplified stand-alone d20 supporting icon for larger sizes. At 16 px, use an appropriately validated simplified mark rather than assuming the numeral remains readable.

Both SVGs are project-authored vector interpretations of the **later approved emerald/gold logo master**, not pixel-for-pixel vectorizations of that image or of the earlier ornate d20 PNG. The later master is archived privately as \`chaon-emerald-d20-approved-master-2026-11-11.jpg\` (1254 × 1254, SHA-256 \`a571081e967f1901564efc94cd817ca101f8c7197e98d24917bb2d3fa587f389\`). Keep the original PNG binaries and their hashes above unchanged. The private master is not an in-repository asset or public CDN.

### GitHub social-preview deployment

![Approved ChaoN social-sharing artwork](social-open-graph-1200x630.png)

The 1200 × 630 PNG is available in the repo, but **committing the image does not activate GitHub's social-preview setting**. Upload/apply it separately in GitHub's repository settings with explicit maintainer authorization. The approved image currently prioritizes Sara and the d20; it has no project-name wordmark. This is an intentional design limitation to consider when reviewing link-sharing appearance.

## Artwork rights and license status

**The three specifically inventoried PNGs above are licensed under [CC BY-NC 4.0](LICENSE-ARTWORK.md) for the rights Dan (Daniel Brown) holds and is authorized to license.** This file-specific notice was adopted on 2026-11-11 and is separate from the repository's AGPL-3.0-only software license.

Noncommercial copying, sharing and adaptations are permitted under the full CC license terms, including attribution and indicating changes. Commercial permissions may be arranged separately with the authorized rightsholder(s). This does not license official branding or endorsements, does not establish copyright in otherwise unprotected AI-generated elements, and does not bind any independently owned third-party rights. The repository's previously included code, documentation and vector SVGs remain subject to their existing AGPL license and any applicable rights.

The **later approved emerald/gold JPG master stored privately**, future imagery, and the two project SVGs are **not** within the three-PNG CC grant. See [the precise covered files and attribution requirements](LICENSE-ARTWORK.md).

## Presentation rules

Keep the README banner restrained and follow it immediately with the product's purpose and safety limitations. Use the d20 as a compact mark and Sara for richer promotional/social artwork; never squeeze the full mascot into a favicon. Preserve the original generated PNGs, independently compare changes before replacing them, and maintain an honest distinction between stored images and activated platform settings.

#!/usr/bin/env python3
"""Selected Gandhi same-binary kind-9 A/B receipt; visual gate, not parity."""

import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image

SAVE_SHA = "b3a54d5c54d2085236e1c156830643f82dfd78853aee5e1b661c32530a6e82ab"
G0_SHA = "b0f36fd73898915cede19acc5bdea43c802c8cd38e4355904511a9b30db99b44"
CAPTURE_SHA = "73315af7a35c70e949421a9f08614a6e3aed5a58201b9647f8b4f3e467bc48ce"
RECORD_SHA = "1530f4cb3104131a4c0719463b73fbc52f2baae52b30c74c8fac816b6180605e"
CROP = (1585, 851, 1905, 1041)
NORTH = (144, 20, 208, 40)
SOUTHEAST = (192, 120, 224, 150)
PRE_ICE = ("minimap-water-gpu.ppm", "minimap-coast-gpu.ppm",
           "minimap-land-gpu.ppm", "minimap-culture-gpu.ppm",
           "minimap-river-gpu.ppm")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def crop(path, box=CROP):
    return np.asarray(Image.open(path).convert("RGB").crop(box), dtype=np.int16)


def error(candidate, original, box=None):
    if box is not None:
        left, top, right, bottom = box
        candidate = candidate[top:bottom, left:right]
        original = original[top:bottom, left:right]
    return int(np.abs(candidate - original).sum())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ("g0", "candidate", "live", "save", "capture", "record"):
        parser.add_argument("--" + key, type=Path, required=True)
    args = parser.parse_args()
    assert sha(args.g0) == G0_SHA and sha(args.save) == SAVE_SHA
    assert sha(args.capture) == CAPTURE_SHA and sha(args.record) == RECORD_SHA
    candidate_info = json.loads((args.candidate / "minimap-surface-gpu.json").read_text())
    live_info = json.loads((args.live / "minimap-surface-gpu.json").read_text())
    assert candidate_info["iceGeometrySource"] == "saved_feature_hex_candidate"
    assert live_info["nativeIceRegionCount"] == 1
    assert live_info["nativeIceTriangles"] == 31042
    assert live_info["nativeIceVertexFormat"] == "short2n_index16"
    for key in ("width", "height", "bounds", "cultureBatches", "cultureTriangles",
                "coastTriangles", "landTriangles", "fogTriangles"):
        assert candidate_info[key] == live_info[key], key
    for name in PRE_ICE:
        assert sha(args.candidate / name) == sha(args.live / name), name
    for directory in (args.candidate, args.live):
        session = json.loads((directory / "ui-session-status.json").read_text())
        assert (session["sourceSha256"] == SAVE_SHA and session["activePlayer"] == 0
                and session["gameTurn"] == 0 and session["liveSessionRestored"] is True)
    provenance = args.live / "selected-gandhi-live-ice-source.json"
    if provenance.is_file():
        source = json.loads(provenance.read_text())
        assert source["recordSha256"] == RECORD_SHA and source["saveSha256"] == SAVE_SHA
        assert source["fullMinimapParityProven"] is False
    original = crop(args.g0)
    candidate = crop(args.candidate / "preview.ppm")
    live = crop(args.live / "preview.ppm")
    assert original.shape == candidate.shape == live.shape == (190, 320, 3)
    whole_candidate, whole_live = error(candidate, original), error(live, original)
    north_candidate, north_live = error(candidate, original, NORTH), error(live, original, NORTH)
    southeast_candidate, southeast_live = (error(candidate, original, SOUTHEAST),
                                            error(live, original, SOUTHEAST))
    changed = int(np.any(candidate != live, axis=2).sum())
    assert changed > 0 and whole_live < whole_candidate and north_live < north_candidate
    print(json.dumps({"schema": "civ5-gandhi-live-kind9-paired-gl-v1", "status": "PASS",
                      "g0Sha256": G0_SHA, "saveSha256": SAVE_SHA,
                      "captureSha256": CAPTURE_SHA, "recordSha256": RECORD_SHA,
                      "mapScreenBoxExclusive": CROP, "mapDimensions": [320, 190],
                      "candidateRgbError": whole_candidate, "liveRgbError": whole_live,
                      "candidateRgbMae": whole_candidate / (320 * 190 * 3),
                      "liveRgbMae": whole_live / (320 * 190 * 3),
                      "northRgbError": [north_candidate, north_live],
                      "southeastRgbError": [southeast_candidate, southeast_live],
                      "changedPixels": changed, "preIceStagesIdentical": True,
                      "fullMinimapParityProven": False, "fullGraphicsParity": False}, indent=2))


if __name__ == "__main__":
    main()

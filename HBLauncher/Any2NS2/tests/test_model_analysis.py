#!/usr/bin/env python3
"""Integration checks for Any2NS2's static model artifacts."""
import json
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
BINARY = ROOT / ("any2ns2.exe" if __import__("os").name == "nt" else "any2ns2")


def run(source_text: str, extension: str = ".cs"):
    with tempfile.TemporaryDirectory(prefix="any2ns2-test-") as td:
        base = pathlib.Path(td)
        source = base / "project"
        output = base / "generated"
        (source / "Assets").mkdir(parents=True)
        (source / ("Fixture" + extension)).write_text(source_text, encoding="utf-8")
        (source / "Assets" / "blob.bin").write_bytes(b"\x00asset\xff")
        icon = ROOT.parent / "apps" / "hello-world" / "assets" / "icon.png"
        subprocess.run([str(BINARY), str(source), str(output), str(icon),
                        'Fixture "Title"', 'Fixture description'], check=True,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        manifest = json.loads((output / "app.json").read_text(encoding="utf-8"))
        assert manifest["name"] == 'Fixture "Title"'
        assert manifest["description"] == "Fixture description"
        assert (output / "assets" / "icon.png").read_bytes() == icon.read_bytes()
        model = json.loads((output / "model.json").read_text(encoding="utf-8"))
        graph = json.loads((output / "model-graph.json").read_text(encoding="utf-8"))
        analysis = json.loads((output / "port-analysis.json").read_text(encoding="utf-8"))
        preserved = (output / "source" / "Assets" / "blob.bin").read_bytes()
        return model, graph, analysis, preserved


def main():
    model, graph, analysis, preserved = run('''namespace Fixture {
      public class Entity { public int Id { get; set; } = 7; }
      public class Player : Entity {
        public Inventory Bag = new Inventory();
        public List<Item> Items = new();
      }
      public class Inventory { public Item Selected; }
      public class Item { public string Name = "item"; }
      public enum Mode { Menu, Playing }
      public class DrawLoop { void Draw() { spriteBatch.DrawString(font, \"hi\", position, color); } void Input() { var pad = GamePad.GetState(PlayerIndex.One); } }
      public class Sound { SoundEffect effect; Microphone microphone; }
    }
''')
    assert preserved == b"\x00asset\xff"
    assert len(model["types"]) == len(graph["nodes"]) == 7
    assert analysis["status"] == "analysis-only"
    symbols = {item["symbol"] for item in analysis["evidence"]}
    assert {"DrawString", "GamePad.GetState", "SoundEffect", "Microphone"} <= symbols
    assert "1920x1080@60" in analysis["target"]["rendering"]
    assert "unverified" in analysis["target"]["audioInput"]
    player = next(item for item in model["types"] if item["name"] == "Player")
    assert player["bases"] == ["Entity"]
    relations = {edge["relation"] for edge in graph["edges"]}
    assert "inherits-or-implements" in relations
    assert "member-reference" in relations
    assert "collection-element" in relations
    assert all(edge["from"] in {node["id"] for node in graph["nodes"]}
               and edge["to"] in {node["id"] for node in graph["nodes"]}
               for edge in graph["edges"])

    model, graph, analysis, preserved = run('export const value = 1;\n', ".js")
    assert model["status"] == "unsupported-language"
    assert graph["status"] == "unsupported-language"
    assert graph["nodes"] == [] and graph["edges"] == []
    assert preserved == b"\x00asset\xff"
    print("PASS: model graph, lexical API hints, audio/1080p capability caveats, unsupported language, byte preservation")


if __name__ == "__main__":
    main()

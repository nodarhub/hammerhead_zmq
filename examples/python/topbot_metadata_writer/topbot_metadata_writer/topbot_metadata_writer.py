import argparse
import shutil
import sys
from pathlib import Path

import yaml
import tifftools
from tqdm import tqdm


def format_details_string(data):
    """Format YAML data into the DETAILS string format for TIFF Software tag."""
    lines = []
    for key, value in data.items():
        if isinstance(value, list):
            formatted_vals = ", ".join(str(float(v)) for v in value)
            lines.append(f"{key}: [{formatted_vals}]")
        else:
            lines.append(f"{key}: {value}")
    content = "\\n".join(lines) + "\\n"
    return f'DETAILS: "{content}"'


def main():
    parser = argparse.ArgumentParser(
        description="Write details YAML metadata into topbot TIFF Software tags"
    )
    parser.add_argument(
        "folder",
        help="Path to folder containing topbot/ and details/ subdirectories",
    )
    args = parser.parse_args()

    folder = Path(args.folder)
    topbot_dir = folder / "topbot"
    details_dir = folder / "details"

    if not topbot_dir.is_dir():
        sys.exit(f"Error: {topbot_dir} not found")
    if not details_dir.is_dir():
        sys.exit(f"Error: {details_dir} not found")

    tiff_files = sorted(topbot_dir.glob("*.tiff"))
    if not tiff_files:
        sys.exit("Error: No TIFF files found in topbot/")

    for tiff_path in tqdm(tiff_files, desc="Writing metadata"):
        yaml_path = details_dir / (tiff_path.stem + ".yaml")
        if not yaml_path.exists():
            tqdm.write(f"  SKIP {tiff_path.name} (no matching YAML)")
            continue

        with open(yaml_path) as f:
            data = yaml.safe_load(f)

        software_value = format_details_string(data)

        # Inject Software tag (305) without rewriting pixel data
        info = tifftools.read_tiff(str(tiff_path))
        for ifd in info["ifds"]:
            ifd["tags"].pop(305, None)
            ifd["tags"][305] = {
                "datatype": tifftools.Datatype.ASCII,
                "data": software_value,
            }

        tmp_path = str(tiff_path) + ".tmp"
        tifftools.write_tiff(info, tmp_path)
        shutil.move(tmp_path, str(tiff_path))

    print("Done")


if __name__ == "__main__":
    main()
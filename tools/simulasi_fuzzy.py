"""Simulasi fuzzy Mamdani yang mengikuti implementasi firmware ESP32."""

from __future__ import annotations

import argparse
import math
from dataclasses import dataclass
from typing import Mapping, Sequence

import numpy as np
import skfuzzy as fuzz


Trap = tuple[float, float, float, float]
Triangle = tuple[float, float, float]
MF = Trap | Triangle

INPUT_NAMES = {
    "suhu": ("Dingin", "Nyaman", "Panas"),
    "kelembaban": ("Kering", "Normal", "Lembab"),
    "orang": ("Sedikit", "Sedang", "Banyak"),
}
OUTPUT_NAMES = ("Rendah", "Sedang", "Tinggi")

# 141 samples: 16.0, 16.1, ..., 30.0, seperti loop firmware.
OUTPUT_UNIVERSE = np.arange(16.0, 30.0 + 0.05, 0.1)

TEMP_MF: Mapping[str, MF] = {
    "dingin": (16.0, 18.0, 22.0, 25.0),
    "nyaman": (23.0, 25.0, 28.0),
    "panas": (27.0, 29.0, 31.0, 35.0),
}
HUMID_MF: Mapping[str, MF] = {
    "kering": (20.0, 30.0, 50.0, 55.0),
    "normal": (50.0, 65.0, 80.0),
    "lembab": (70.0, 80.0, 90.0, 100.0),
}
OCC_MF: Mapping[str, MF] = {
    "sedikit": (0.0, 1.0, 3.0, 4.0),
    "sedang": (3.0, 4.0, 7.0),
    "banyak": (6.0, 8.0, 9.0, 10.0),
}
SETPOINT_MF: Mapping[str, MF] = {
    "rendah": (17.0, 18.0, 22.0, 24.0),
    "sedang": (23.0, 25.0, 30.0),
    "tinggi": (25.0, 30.0, 32.0, 33.0),
}

# rules[suhu][kelembaban][hunian], 0=Rendah, 1=Sedang, 2=Tinggi.
RULES = (
    ((2, 2, 1), (2, 2, 1), (1, 1, 0)),
    ((2, 2, 1), (1, 1, 0), (1, 1, 1)),
    ((1, 1, 0), (1, 1, 0), (0, 0, 0)),
)


@dataclass(frozen=True)
class FuzzyResult:
    crisp_setpoint: float
    setpoint_int: int
    firing: tuple[float, float, float]
    memberships: dict[str, dict[str, float]]


def trap_membership(x: float, points: Trap) -> float:
    """Evaluate a trapezoid using the firmware's strict outer boundaries."""
    a, _, _, d = points
    if x <= a or x >= d:
        return 0.0
    values = fuzz.trapmf(np.asarray([x], dtype=float), np.asarray(points, dtype=float))
    return float(values[0])


def triangle_membership(x: float, points: Triangle) -> float:
    """Evaluate a triangle using the firmware's strict outer boundaries."""
    a, _, c = points
    if x <= a or x >= c:
        return 0.0
    values = fuzz.trimf(np.asarray([x], dtype=float), np.asarray(points, dtype=float))
    return float(values[0])


def _membership_for_input(value: float, definitions: Mapping[str, MF]) -> dict[str, float]:
    result: dict[str, float] = {}
    for name, points in definitions.items():
        if len(points) == 4:
            result[name] = trap_membership(value, points)  # type: ignore[arg-type]
        else:
            result[name] = triangle_membership(value, points)  # type: ignore[arg-type]
    return result


def _aggregate_firing(memberships: Mapping[str, Mapping[str, float]]) -> tuple[float, float, float]:
    temp = [memberships["suhu"][name] for name in ("dingin", "nyaman", "panas")]
    humid = [memberships["kelembaban"][name] for name in ("kering", "normal", "lembab")]
    occ = [memberships["orang"][name] for name in ("sedikit", "sedang", "banyak")]
    firing = [0.0, 0.0, 0.0]

    for i in range(3):
        for j in range(3):
            for k in range(3):
                value = min(temp[i], humid[j], occ[k])
                label = RULES[i][j][k]
                firing[label] = max(firing[label], value)

    return tuple(firing)


def _defuzzify(firing: Sequence[float]) -> float:
    if sum(firing) == 0.0:
        return 23.0

    output_low = np.minimum(
        firing[0], fuzz.trapmf(OUTPUT_UNIVERSE, SETPOINT_MF["rendah"])
    )
    output_mid = np.minimum(
        firing[1], fuzz.trimf(OUTPUT_UNIVERSE, SETPOINT_MF["sedang"])
    )
    output_high = np.minimum(
        firing[2], fuzz.trapmf(OUTPUT_UNIVERSE, SETPOINT_MF["tinggi"])
    )
    aggregate = np.maximum.reduce((output_low, output_mid, output_high))
    denominator = float(np.sum(aggregate))
    if denominator == 0.0:
        return 23.0
    return float(np.sum(OUTPUT_UNIVERSE * aggregate) / denominator)


def round_firmware(value: float) -> int:
    """Match Arduino round() for the positive setpoint range."""
    return int(math.floor(value + 0.5))


def compute_fuzzy(suhu: float, kelembaban: float, orang: float) -> FuzzyResult:
    """Compute one fuzzy case; sensor inputs are already calibrated."""
    inputs = (suhu, kelembaban, orang)
    if not all(math.isfinite(value) for value in inputs):
        raise ValueError("Input suhu, kelembaban, dan jumlah orang harus berupa angka valid.")

    memberships = {
        "suhu": _membership_for_input(suhu, TEMP_MF),
        "kelembaban": _membership_for_input(kelembaban, HUMID_MF),
        "orang": _membership_for_input(orang, OCC_MF),
    }
    firing = _aggregate_firing(memberships)
    crisp = min(max(_defuzzify(firing), 16.0), 30.0)

    return FuzzyResult(
        crisp_setpoint=crisp,
        setpoint_int=round_firmware(crisp),
        firing=firing,
        memberships=memberships,
    )


def _format_result(
    suhu: float,
    kelembaban: float,
    orang: float,
    result: FuzzyResult,
    hardware_setpoint: int | None,
) -> str:
    lines = [
        "=== Simulasi Fuzzy Mamdani ===",
        "Input (sudah terkalibrasi)",
        f"  Suhu        : {suhu:.2f} C",
        f"  Kelembaban  : {kelembaban:.2f} %",
        f"  Jumlah orang: {orang:.2f}",
        "",
        "Membership input",
        "  Suhu       | "
        + " | ".join(
            f"{name}: {result.memberships['suhu'][key]:.4f}"
            for key, name in zip(("dingin", "nyaman", "panas"), INPUT_NAMES["suhu"])
        ),
        "  Kelembaban | "
        + " | ".join(
            f"{name}: {result.memberships['kelembaban'][key]:.4f}"
            for key, name in zip(
                ("kering", "normal", "lembab"), INPUT_NAMES["kelembaban"]
            )
        ),
        "  Hunian     | "
        + " | ".join(
            f"{name}: {result.memberships['orang'][key]:.4f}"
            for key, name in zip(("sedikit", "sedang", "banyak"), INPUT_NAMES["orang"])
        ),
        "",
        "Firing strength output",
        "  " + " | ".join(
            f"{name}: {value:.4f}" for name, value in zip(OUTPUT_NAMES, result.firing)
        ),
        "",
        "Output",
        f"  Setpoint crisp   : {result.crisp_setpoint:.4f} C",
        f"  Setpoint simulasi: {result.setpoint_int} C",
    ]

    if hardware_setpoint is not None:
        comparison = "COCOK" if result.setpoint_int == hardware_setpoint else "BERBEDA"
        lines.extend(
            [
                f"  Setpoint hardware : {hardware_setpoint} C",
                f"  Perbandingan      : {comparison}",
            ]
        )

    return "\n".join(lines)


def _prompt_float(label: str) -> float:
    return float(input(f"{label}: ").strip())


def _read_batch_rows() -> list[tuple[float, float, float, int]]:
    print("Tempel baris dengan format: suhu kelembaban jumlah_orang setpoint_hardware")
    print("Akhiri input dengan baris kosong atau EOF.")
    rows: list[tuple[float, float, float, int]] = []

    while True:
        try:
            line = input().strip()
        except EOFError:
            break
        if not line:
            break

        fields = line.split()
        if len(fields) != 4:
            raise ValueError(
                f"Baris ke-{len(rows) + 1} harus memiliki 4 kolom: "
                "suhu kelembaban jumlah_orang setpoint_hardware."
            )

        try:
            suhu, kelembaban, orang = map(float, fields[:3])
            hardware_setpoint = int(fields[3])
        except ValueError as error:
            raise ValueError(f"Baris ke-{len(rows) + 1} berisi angka yang tidak valid.") from error

        if not 16 <= hardware_setpoint <= 30:
            raise ValueError(
                f"Setpoint hardware pada baris ke-{len(rows) + 1} harus 16 sampai 30."
            )
        rows.append((suhu, kelembaban, orang, hardware_setpoint))

    if not rows:
        raise ValueError("Tidak ada data batch yang dimasukkan.")
    return rows


def _format_batch_result(rows: Sequence[tuple[float, float, float, int]]) -> str:
    lines = [
        "=== Hasil Batch Simulasi Fuzzy Mamdani ===",
        "No  Suhu      Humid     Orang  HW  Simulasi  Crisp      Status",
        "--  --------  --------  -----  --  --------  ---------  -------",
    ]
    cocok = 0

    for index, (suhu, kelembaban, orang, hardware_setpoint) in enumerate(rows, start=1):
        result = compute_fuzzy(suhu, kelembaban, orang)
        status = "COCOK" if result.setpoint_int == hardware_setpoint else "BERBEDA"
        cocok += status == "COCOK"
        lines.append(
            f"{index:>2}  {suhu:>8.3f}  {kelembaban:>8.3f}  {orang:>5.0f}  "
            f"{hardware_setpoint:>2}  {result.setpoint_int:>8}  "
            f"{result.crisp_setpoint:>9.4f}  {status}"
        )

    total = len(rows)
    persentase = cocok / total * 100.0
    lines.extend(
        [
            "",
            f"Ringkasan: {cocok}/{total} cocok ({persentase:.2f}%).",
            "Perbandingan menggunakan setpoint integer hasil pembulatan.",
        ]
    )
    return "\n".join(lines)


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Simulasi fuzzy Mamdani kontrol AC")
    parser.add_argument(
        "--batch",
        action="store_true",
        help="Membaca banyak baris: suhu kelembaban jumlah_orang setpoint_hardware",
    )
    parser.add_argument("--suhu", type=float, help="Suhu terkalibrasi dalam C")
    parser.add_argument("--kelembaban", type=float, help="Kelembaban terkalibrasi dalam persen")
    parser.add_argument("--orang", type=float, help="Jumlah orang")
    parser.add_argument("--hardware-setpoint", type=int, help="Setpoint fuzzy dari hardware")
    args = parser.parse_args(argv)

    try:
        if args.batch:
            print(_format_batch_result(_read_batch_rows()))
            return 0

        interactive = args.suhu is None or args.kelembaban is None or args.orang is None
        suhu = args.suhu if args.suhu is not None else _prompt_float("Suhu terkalibrasi (C)")
        kelembaban = (
            args.kelembaban
            if args.kelembaban is not None
            else _prompt_float("Kelembaban terkalibrasi (%)")
        )
        orang = args.orang if args.orang is not None else _prompt_float("Jumlah orang")

        hardware_setpoint = args.hardware_setpoint
        if interactive and hardware_setpoint is None:
            text = input("Setpoint hardware (boleh kosong): ").strip()
            hardware_setpoint = int(text) if text else None

        if hardware_setpoint is not None and not 16 <= hardware_setpoint <= 30:
            raise ValueError("Setpoint hardware harus berada pada rentang 16 sampai 30 C.")

        result = compute_fuzzy(suhu, kelembaban, orang)
    except (ValueError, EOFError) as error:
        print(f"Input tidak valid: {error}")
        return 2

    print(_format_result(suhu, kelembaban, orang, result, hardware_setpoint))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# Board transmit-power specification audit

Checked 2026-10-23 against manufacturer documentation for all 11 firmware targets. The implementation was already committed in `99986c9` when this audit started.

## Results

The existing maximum settings do not require a change. Distinguish the programmable SX1262 drive level from the board's conducted RF output specification. A 22 dBm chip setting does not guarantee 22 dBm at the antenna connector.

| Firmware target(s) | Current maximum selection, before regional limits | Manufacturer evidence and interpretation |
| --- | --- | --- |
| `heltec-v3` | 22 dBm chip drive | [Heltec comparison](https://wiki.heltec.org/docs/devices/open-source-hardware/esp32-series/lora-32/wifi-lora-32-v4/) specifies **21 ±1 dBm** board output. The chip setting is within the SX1262's supported range and 22 dBm is the upper end of the board's stated output range, not its nominal rating. |
| `heltec-v4` (V4.2 GC1109) | Estimated 28 dBm output, 20 dBm chip drive | [Heltec V4 documentation](https://wiki.heltec.org/docs/devices/open-source-hardware/esp32-series/lora-32/wifi-lora-32-v4/) specifies **28 ±1 dBm** for the amplified version. |
| `heltec-v4-3` (V4.3 KCT8103L) | Estimated 28 dBm output, 17 dBm chip drive | Same **28 ±1 dBm** amplified-board rating; [hardware revision log](https://wiki.heltec.org/docs/devices/open-source-hardware/esp32-series/lora-32/wifi-lora-32-v4/hardware-update-log) identifies the FEM change. |
| `t-deck` | 22 dBm chip drive | [LILYGO T-Deck specifications](https://lilygo.cc/en-de/products/t-deck) specify **+22 dBm** transmit power. |
| `t-deck-plus` | 22 dBm chip drive | [LILYGO T-Deck Plus specifications](https://lilygo.cc/en-de/products/t-deck-plus-1) specify **+22 dBm** transmit power. |
| `t-deck-pro`, `t-deck-pro-v1-1`, `t-deck-pro-max` | 22 dBm chip drive | [LILYGO Pro documentation](https://wiki.lilygo.cc/products/t-deck-series/t-deck-pro/) identifies SX1262 and the three hardware branches. The [SX1262 manufacturer](https://www.semtech.com/products/wireless-rf/lora-connect/sx1262) specifies **+22 dBm**. This verifies the chip ceiling; the checked Pro documentation does **not** separately publish conducted output ratings for each board revision. Do not confuse the e-paper Pro MAX branch with the separate T-Deck MAX LCD product. |
| `rak4631`, `rak4631-s140-6-1-1` | 22 dBm chip drive | [RAK4631 datasheet](https://docs.rakwireless.com/product-categories/wisblock/rak4631/datasheet/) specifies **up to 22 dBm** LoRa output. SoftDevice versions do not change RF hardware. |
| `heltec-t114` | 22 dBm chip drive | [Heltec T114 datasheet](https://s.heltec.cn/download/Mesh_Node_T114/Datasheet.pdf), section 3.3.1, PDF page 9, specifies **21 ±1 dBm** in all three listed frequency bands. As with V3, 22 dBm is the chip setting and upper end of the stated range, not a guaranteed nominal conducted output. |

## Amplifier and regional limits

The two V4 targets explicitly assume the named external FEM. Heltec also sells a low-power V4 option; the 28 dBm assumption must not be applied to that option merely because its board is called V4.

FEM drive values above come from the existing gain model, not from a manufacturer-certified per-drive output calibration. Maximum-output specifications confirm the board ceilings but do not independently validate every point on that model. Supply, frequency, RF losses, and amplifier tolerances affect actual output. Physical conducted-power measurement remains outstanding.

The preset ceilings remain US 30 dBm, EU_868 27 dBm, and EU_433 10 dBm. Selection uses the highest modeled output within both the hardware ceiling and preset ceiling. A tolerance specification such as 28 ±1 dBm is not a reason to increase the nominal target to 29 dBm.

## Verification

`python3 tools/test_radio_power.py` passes for all 11 environments using their resolved PlatformIO defines. It verifies chip/FEM profile selection and the 30, 27, and 10 dBm preset ceilings. This is a software check, not an RF measurement. No firmware source changes were needed for this specification audit.

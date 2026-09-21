#!/usr/bin/env python3
"""
Test HAR move triggering by generating REC files and running them through the game.

This script:
1. Scans all HAR AI config files
2. Extracts move sequences (charge_moves, push_moves, projectile_moves)
3. Generates REC files for each move using gen_move_test
4. Runs them through the game engine to validate triggering
"""

import json
import subprocess
import sys
from pathlib import Path
from collections import defaultdict


HAR_NAMES = {
    0: "chronos",
    1: "electra",
    2: "thorn",
    3: "flail",
    4: "gargoyle",
    5: "jaguar",
    6: "katana",
    7: "nova",
    8: "pyros",
    9: "shadow",
    10: "shredder",
}

# Map direction notation to ACT_* hex values (from controller.h)
DIRECTION_MAP = {
    'F': 0x40,  # ACT_RIGHT
    'B': 0x20,  # ACT_LEFT
    'D': 0x10,  # ACT_DOWN
    'U': 0x08,  # ACT_UP
    'DF': 0x50,  # DOWN | RIGHT
    'DB': 0x30,  # DOWN | LEFT
    'UF': 0x48,  # UP | RIGHT
    'UB': 0x28,  # UP | LEFT
    '5': 0x01,  # ACT_STOP (neutral)
}

BUTTON_MAP = {
    'P': 0x04,  # ACT_PUNCH
    'K': 0x02,  # ACT_KICK
}


class MoveTestGenerator:
    def __init__(self, build_dir, openomf_bin=None):
        self.build_dir = Path(build_dir)
        self.openomf_bin = openomf_bin or (self.build_dir / "openomf")
        self.gen_move_test = self.build_dir / "gen_move_test"
        self.resources_dir = self.build_dir / "resources"
        self.config_dir = None
        
        # Find config directory (might be in resources or in openomf source)
        possible_paths = [
            Path.cwd() / "resources" / "ai_config",
            self.resources_dir / "ai_config",
            Path.cwd() / "resource" / "ai_config",
        ]
        for path in possible_paths:
            if path.exists():
                self.config_dir = path
                break
        
        if not self.config_dir:
            raise RuntimeError(f"Could not find ai_config directory")
    
    def parse_sequence(self, sequence):
        """
        Parse a move sequence like ["F", "5", "F+P"] into action bytes.
        Returns list of hex bytes.
        """
        actions = []
        for token in sequence:
            action_byte = 0
            
            # Split by '+' for compound inputs
            if '+' in token:
                parts = token.split('+')
                for part in parts:
                    if part in DIRECTION_MAP:
                        action_byte |= DIRECTION_MAP[part]
                    elif part in BUTTON_MAP:
                        action_byte |= BUTTON_MAP[part]
            else:
                if token in DIRECTION_MAP:
                    action_byte = DIRECTION_MAP[token]
                elif token in BUTTON_MAP:
                    action_byte = BUTTON_MAP[token]
            
            actions.append(f"{action_byte:02x}")
        
        return actions
    
    def list_all_hars(self):
        """Find all HAR config files."""
        har_dir = self.config_dir / "hars"
        if not har_dir.exists():
            return []
        
        hars = []
        for config_file in sorted(har_dir.glob("*.json")):
            hars.append(config_file.stem)  # Just the filename without .json
        return hars
    
    def get_all_moves(self, har_name):
        """
        Extract all moves from a HAR config.
        Returns: {move_type: [(name, sequence), ...], ...}
        """
        config_file = self.config_dir / "hars" / f"{har_name}.json"
        if not config_file.exists():
            return {}
        
        try:
            with open(config_file) as f:
                config = json.load(f)
        except Exception as e:
            print(f"Error loading {har_name}: {e}", file=sys.stderr)
            return {}
        
        moves = {}
        for move_type in ["charge_moves", "push_moves", "projectile_moves"]:
            moves[move_type] = []
            for move in config.get(move_type, []):
                name = move.get("name", "unknown")
                sequence = move.get("sequence", [])
                if sequence:
                    moves[move_type].append((name, sequence))
        
        return moves
    
    def run_test_rec(self, rec_file):
        """
        Run a REC file through the game engine.
        Returns True if execution succeeded.
        """
        try:
            result = subprocess.run(
                [
                    str(self.openomf_bin),
                    "--force-audio-backend=NULL",
                    "--force-renderer=NULL",
                    "--speed=10",
                    "-P", str(rec_file)
                ],
                capture_output=True,
                timeout=30,
                cwd=str(self.build_dir)
            )
            return result.returncode == 0
        except subprocess.TimeoutExpired:
            return False
        except Exception as e:
            print(f"Error running REC: {e}", file=sys.stderr)
            return False
    
    def generate_rec_for_move(self, har_id, opponent_id, move_name, sequence, output_file):
        """
        Generate a REC file for a specific move using gen_move_test.
        Returns True if generation succeeded.
        """
        try:
            actions = self.parse_sequence(sequence)
            cmd = [str(self.gen_move_test), str(har_id), str(opponent_id), str(output_file)] + actions
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
            return result.returncode == 0
        except Exception as e:
            print(f"Error generating REC: {e}", file=sys.stderr)
            return False


def main():
    build_dir = Path.cwd() / "build"
    openomf_bin = build_dir / "openomf"
    
    if not build_dir.exists():
        print(f"Build directory not found: {build_dir}")
        sys.exit(1)
    
    if not openomf_bin.exists():
        print(f"OpenOMF binary not found: {openomf_bin}")
        sys.exit(1)
    
    generator = MoveTestGenerator(str(build_dir), str(openomf_bin))
    
    # Discover all moves
    all_hars = generator.list_all_hars()
    print(f"Found {len(all_hars)} HARs")
    
    test_cases = []
    for har_name in all_hars:
        har_id = None
        for hid, hname in HAR_NAMES.items():
            if hname == har_name:
                har_id = hid
                break
        
        if har_id is None:
            continue
        
        moves = generator.get_all_moves(har_name)
        for move_type, move_list in moves.items():
            for move_name, sequence in move_list:
                test_cases.append((har_id, har_name, move_type, move_name, sequence))
    
    print(f"\nFound {len(test_cases)} test cases")
    print("\nTest cases by HAR:")
    by_har = defaultdict(list)
    for har_id, har_name, move_type, move_name, sequence in test_cases:
        by_har[har_name].append(move_name)
    
    for har_name in sorted(by_har.keys()):
        print(f"  {har_name}: {', '.join(by_har[har_name])}")
    
    # Generate and run tests if requested
    if len(sys.argv) > 1 and sys.argv[1] == "--run":
        print("\n" + "="*60)
        print("Generating and testing RECs...")
        print("="*60 + "\n")
        
        test_dir = Path("/tmp/openomf_move_tests")
        test_dir.mkdir(exist_ok=True)
        
        passed = 0
        failed = 0
        
        for har_id, har_name, move_type, move_name, sequence in sorted(test_cases):
            rec_name = f"{har_name}_{move_type}_{move_name}.rec"
            rec_file = test_dir / rec_name
            
            # Opponent is always HAR 0 (Chronos)
            opponent_id = 0
            
            if generator.generate_rec_for_move(har_id, opponent_id, move_name, sequence, str(rec_file)):
                print(f"✓ {har_name:12} {move_type:18} {move_name:25}", end=" ")
                
                if generator.run_test_rec(str(rec_file)):
                    print("[PASS]")
                    passed += 1
                else:
                    print("[RUN FAILED]")
                    failed += 1
            else:
                print(f"✗ {har_name:12} {move_type:18} {move_name:25} [GEN FAILED]")
                failed += 1
        
        print(f"\n{'='*60}")
        print(f"Results: {passed} passed, {failed} failed")
        print(f"{'='*60}")


if __name__ == "__main__":
    main()

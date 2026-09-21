# OMF 2097 Reference Guide

**Source**: Comprehensive FAQ by Robyrt from Neoseeker  
**Date**: Last updated May 7, 2004 (Version 1.5)  
**Purpose**: Reference for defining pilot stats and understanding existing moves in OpenOMF

---

## Pilot Profiles & Stats

### Overview of Pilot Stats
- **Power**: Raw damage output
- **Agility**: Speed and jump height
- **Endurance**: Stun bar and health bar total
- All pilots have 30 total stat points to distribute (with some variation)
- Stats can be redistributed in-game while keeping the same total

### Pilot Stats Reference

| Pilot | Age | Specialty | Power | Agility | Endurance | Notes |
|-------|-----|-----------|-------|---------|-----------|-------|
| Crystal | 23 | Genetic Engineering | 5 | 16 | 9 | Fast, good for speed-based robots (Jaguar, Shredder, Gargoyle) |
| Steffan | 17 | Sales/Marketing | 13 | 9 | 8 | High power, aggressive style, good combo potential |
| Milano | 35 | Security, Kick Boxing | 7 | 20 | 4 | Fastest pilot, fragile, most similar to tournament mode |
| Christian | 23 | Genetic Engineering, Jujitsu | 9 | 7 | 15 | Aggressive, high endurance, learns quickly |
| Shirro | 73 | Public Relations, Karate | 20 | 1 | 8 | Highest power, slowest, relies on powerful attacks |
| Jean-Paul | 27 | Market Analyst | 9 | 10 | 11 | Well-rounded, defensive, prefers special moves |
| Ibrahim | 48 | Robotics Engineer | 10 | 1 | 20 | Extremely slow, high endurance, defensive |
| Angel | Unknown | Unknown | 7 | 10 | 13 | Mystery character, mysterious backstory, likes jump moves |
| Cossette | 39 | Space Station Design | 14 | 8 | 8 | Defensive, powerful when attacking, formerly crippled |
| Raven | 26 | Bodyguard, Kickboxing | 14 | 4 | 12 | High power, low agility, prefers special moves and throws |
| Major Kreissack | 103 | President of WAR | 16 | 15 | 16 | Boss character, super-buffed stats, not selectable |

**Special Notes:**
- Shirro has **slightly fewer total stats** than most pilots
- Milano, Christian, and Ibrahim have **slightly more total stats** than others
- Crystal is the recommended beginner/fast pilot
- Shirro demonstrates the glass cannon archetype (high power, low agility)

---

## Robot Profiles & Core Stats

### Robot Stats Breakdown

| Robot | Creator | Power | Speed | Armor | Stun Resistance | Jump Speed | Fall Speed | Special Notes |
|-------|---------|-------|-------|-------|-----------------|------------|-----------|----------------|
| Jaguar | Ibrahim Hothe | Mid | Highest | 100 | 27.5 | 14.5 | 1.15 | Speed/dexterity focus, good at throwing |
| Shadow | Unknown | Low | 4.7 | 95 | 22.5 | 13.5 | 0.95 | Secretive, special grab mechanics |
| Thorn | - | High | 5 | 105 | 27.5 | 14.5 | 1.15 | Power-focused, predictable |
| Pyros | James Sweeney | Mid | 4.3 | 110 | 27.5 | 12.5 | 0.88 | Flame-based, slow, defensive |
| Electra | Cossette Akira | Mid | 4.8 | 95 | 27.5 | 13.5 | 1.13 | Priority-based, high-speed pokes |
| Katana | - | Mid | 4.2 | 100 | 27.5 | 12.5 | 0.88 | Invincible Rising Blade, excellent combos |
| Shredder | Marcus Knight | Mid | 4.8 | 105 | 27.5 | 13.5 | 1 | Fastest combos, Flying Hands infinite |
| Flail | Stephen Jamison | High | 4.4 | 105 | 27.5 | 12.3 | 0.9 | Chains, Charging Punch, original mechanics |
| Gargoyle | Marcus Knight | Mid | 4 | 95 | 27.5 | 12.5 | 0.8 | Air maneuverability, Tri-jump combo |
| Chronos | - | Low | 4.3 | 100 | 27.5 | 13 | 1.05 | Teleport, Matter Phasing, Stasis Activator |
| Nova | - | Mid | 4.9 | 115 | 27.5 | 13.5 | 1 | Largest robot, projectiles, slow |

---

## Move Categories & Mechanics

### Basic Attack Types

| Type | Block Height | Properties | Notes |
|------|---------------|-----------|-------|
| **Light** | High | Never causes wall hit | Jabs, weak strikes |
| **Medium** | High | Wall hit only in Powerplant arena | Standard attacks |
| **Heavy** | High | Wall hit in all arenas | Strong strikes, knockdowns |
| **Low** | Low | Must be blocked low, no wall hit | Sweeps, low kicks |
| **Jump** | High | Allows air recovery, rebounds on early hit | Aerial attacks |
| **Throw** | - | Unblockable, knocks down | Proximity-based |

### Common Move Patterns Across Robots

**Jump Attacks**: (air) P, (air) K - Standard jump punch and kick, variations per robot  
**Standing Strikes**: P, b+P, K, b+K - Varies by robot from 4 to 20+ damage  
**Crouching Strikes**: df+P, d+P, db+P, df+K, d+K, db+K  
**Special Moves**: Quarter-circle forward (qcf), Quarter-circle back (qcb), Half-circle variants  
**Throws**: (close) f+P or (close) b+P - 20-25 damage typically

---

## Notable Moves by Robot

### Jaguar
- **Jaguar Leap** (qcf+P) - Medium KD, good combo starter
- **Shadow Jaguar Leap** (hcf+P) - Double-hit version
- **Concussion Cannon** (qcb+P) - Projectile, can be directed
- **Overhead Throw** (air) d+P - Air throw, invincibility properties
- **Suplex** (close) f+P - Ground throw

### Shadow  
- **Shadow Punch** (qcb+P) - Creates shadow projectiles
- **Shadow Kick** (qcb+K) - Low damage, good for setup
- **Shadow Grab** (d,d+P) - Capture move, unblockable coming down
- **Kick Throw** (close) f+P - Standard throw

### Thorn
- **Spike Charge** (f,f+P) - Forward charge throw, auto-triggers on low hit
- **Speed Kick** (qcf+K) - Excellent anti-air, safe when blocked
- **Shadow Speed Kick** (hcf+K) - Double-hit version

### Pyros
- **Thrust Attack** (f,f+P) - Throw counterable, medium damage
- **Fire Spin** (d,P) - Multi-hit projectile, enhanced version available
- **Jet Swoop** (air) d+K - Vertical descent, combo starter

### Electra
- **Electric Shards** (qcf+K) - Projectiles, high priority, multiple hits with enhancements
- **Rolling Thunder** (f,f+P) - Medium KD, rebounds when blocked
- **Ball Lightning** (qcb+P) - Projectile, directional control in Hyper Mode

### Katana
- **Rising Blade** (qcf+P) - Fast, invincible, combo starter (2 hits, KD on second)
- **Shadow Rising Blade** (hcf+P) - 3-hit version with enhancements
- **Razor Spin** (qcf+K or qcb+K) - Jump attack, rebounds on hit
- **Head Stomp** (air) d+K - Light AR, anti-air jump attack
- **Fireball** (d,db,b+P) - Requires 3rd enhancement

### Shredder
- **Headbutt** (qcf+P) - Medium KD, combo starter
- **Shadow Headbutt** (qcf,f+P) - Heavy KD version
- **Flip Kick** (d,d+K) - Invincible to projectiles, shadow move properties
- **Flying Hands** (qcb+P) - Projectile, counterable, can create infinite

### Flail
- **Swinging Chains** (d,P or d,K) - Multiple hits, can be controlled with enhancements
- **Charging Punch** (b,b+P) - Heavy KD, powerful
- **Shadow Charging Punch** (qcb,b+P) - Enhanced version
- **Spinning Throw** (f,f+K) - Throw that hits while airborne

### Gargoyle
- **Rising Talon** (qcf+P) - Light KD, high priority anti-air
- **Wing Charge** (f,f+P) - Horizontal attack, cancelable
- **Air Wing Charge** (air) f,f+P - Aerial version with enhancement
- **Diving Claw** (air) d+K - Heavy KD, aerial finishing move

### Chronos
- **Teleport** (d,P) - Phase behind opponent, directional variants
- **Matter Phasing** (qcb+K) - Escape move, medium KD output
- **Stasis Activator** (qcf+P or hcf+P) - Freezes opponent in place

### Nova
- **Belly Flop** (air) d+P - Heavy, stuns grounded opponents
- **Missile Launcher** (d,df,f+P) - Projectile, can be airborne
- **Mini Grenade** (qcb+P) - Medium KD, good anti-air
- **Earthquake Smash** (d,d+P) - Unblockable KD, special properties

---

## Difficulty Levels & AI Behavior

### Difficulty Scale
0. **Punching Bag** - Heavily reduced AI capability
1. **Rookie** - Very basic combos and movement
2. **Veteran** - Standard difficulty
3. **World Class** - Competent AI
4. **Champion** - Advanced tactics
5. **Deadly** - Expert combos, scrap/destruction moves on defeat
6. **Ultimate** - Superior AI, rehit mode combos, 5% block damage penalty

---

## Tournament Information

### Tournaments Available
1. **North American Open** - Introductory tournament, good for earning cash
2. **Katushai Challenge** - Mid-level, introduces varied robots and characters
3. **WAR Invitational** - High-level competition with specialized fighters
4. **World Championship** - Ultimate challenge with 30+ opponents

### Progression System
- Start with basic stats and one robot
- Earn cash from victories (based on opponent rank, damage, points)
- Upgrade personal stats (Power, Agility, Endurance)
- Upgrade robot stats (Arm/Leg Power/Speed, Armor, Stun Resistance)
- Trade in robots for new ones (85% cash refund on upgrades)

---

## Combat Mechanics Summary

### Key Systems
- **Rehit Mode**: Hit airborne opponents multiple times with damage penalty
- **Air Recovery (AR)**: Recover mid-air after being hit
- **Wall Hits**: Enhanced damage when opponent hits arena wall
- **Stun System**: Filled by damage, causes temporary immobilization at 0
- **Hyper Mode**: Enables advanced special moves and projectile control

### Combo Notation
- P/K = Punch/Kick
- d+P = Down + Punch (crouch)
- f+P = Forward + Punch (standing)
- b+P = Back + Punch
- qcf = Quarter-circle forward (df→f)
- hcf = Half-circle forward (db→d→df→f)
- xx = Cancel previous move
- , = One after another
- + = Simultaneously
- (air) = Must be jumping
- (close) = Must be within throw range

---

## Key Takeaways for Pilot/Robot Design

1. **Stat Distribution**: Most pilots have ~30 stat points (with some variance for balance)
2. **Power vs Agility**: Creates distinct playstyles (glass cannon vs tank)
3. **Endurance**: Critical for surviving combo pressure
4. **Robot Speed**: Affects frame data, spacing, and recovery
5. **Stun Resistance**: Determines how quickly stun bar fills from damage
6. **Special Moves**: Each robot has signature mechanics (projectiles, teleport, captures)
7. **Combos**: Damage is dramatically increased through proper chaining
8. **Throws**: Balanced close-range tools with knockdown properties
9. **Difficulty Scaling**: AI should access appropriate move sets and tactics per level
10. **Enhancements**: Optional upgrades that add extra hits or properties to moves


# Implémentation d’un système de communication sans fil multi-modalités entre microcontrôleurs STM32F4

**Projet**: Introduction aux Microcontrôleurs (L3 Informatique)
**Groupe**: grp1 td-d
**Date**: 31/03/2025

## Description
Ce projet vise à transmettre un message en code Morse entre deux STM32F4 via un buzzer et un capteur sonore.

## Structure
- **docs/report.md**: Rapport détaillé du projet
- **src/transmitter**: Code pour l’émetteur (MC1)
- **src/receiver**: Code pour le récepteur (MC2)

## Installation & Build
1. Ouvrir `stm32f4-morse-comm/src` dans STM32CubeIDE
2. Importer chaque sous-dossier comme projet séparé
3. Compiler et flasher sur carte STM32F4

## Usage
1. Connecter PC émetteur et PC récepteur via UART (ex. CoolTerm)
2. Envoyer un texte sur le PC émetteur, visualiser le décodage sur le PC récepteur.

.syntax unified
.cpu cortex-m4
.thumb

.global assembly_mash_counter

assembly_mash_counter:
    MOV R0, #0      @ default return = 0
    BX LR

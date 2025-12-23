/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/23 17:59:21 by jainavas          #+#    #+#             */
/*   Updated: 2025/12/23 17:59:31 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/woody.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <binary>\n", argv[0]);
        return 1;
    }
    
    t_elf elf = {0};
    
    // Parsear el ELF
    if (parse_elf(argv[1], &elf) < 0)
        return 1;
    
    printf("ELF parsed successfully!\n");
    printf("Entry point: 0x%lx\n", elf.ehdr->e_entry);
    printf("Sections: %d\n", elf.ehdr->e_shnum);
    
    // Buscar la sección .text
    Elf64_Shdr *text = find_section(&elf, ".text");
    if (!text) {
        fprintf(stderr, "Error: .text section not found\n");
        cleanup_elf(&elf);
        return 1;
    }
    
    printf(".text found at offset 0x%lx, size: %lu bytes\n",
           text->sh_offset, text->sh_size);
    
    // Generar clave
    uint8_t key[KEY_SIZE];
    generate_key(key, KEY_SIZE);
    
    printf("Generated key: ");
    for (int i = 0; i < KEY_SIZE; i++)
        printf("%02X", key[i]);
    printf("\n");
    
    // TODO: empaquetar el binario
    // pack_elf(&elf, key);
    
    cleanup_elf(&elf);
    return 0;
}
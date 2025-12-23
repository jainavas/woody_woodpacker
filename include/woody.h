/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   woody.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/23 17:49:52 by jainavas          #+#    #+#             */
/*   Updated: 2025/12/23 18:00:30 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WOODY_H
#define WOODY_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <elf.h>

// Tamaño de la clave RC4
#define KEY_SIZE 16

// Estructuras
typedef struct {
    void        *data;       // Contenido del archivo ELF mapeado
    size_t      size;        // Tamaño total del archivo
    Elf64_Ehdr  *ehdr;       // Puntero al ELF header
    Elf64_Phdr  *phdr;       // Puntero a program headers
    Elf64_Shdr  *shdr;       // Puntero a section headers
} t_elf;

// elf_parser.c
int     parse_elf(const char *filename, t_elf *elf);
void    cleanup_elf(t_elf *elf);
Elf64_Shdr *find_section(t_elf *elf, const char *name);

// rc4.c
void    rc4_init(uint8_t *S, uint8_t *key, size_t key_len);
void    rc4_crypt(uint8_t *S, uint8_t *data, size_t len);
void    generate_key(uint8_t *key, size_t len);

// packer.c
int     pack_elf(t_elf *elf, uint8_t *key);

#endif
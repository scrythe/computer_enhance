const std = @import("std");
const Io = std.Io;

const reg_bits_name_map = [8]*const [2]u8{
    "al",
    "cl",
    "dl",
    "bl",
    "ah",
    "ch",
    "dh",
    "bh",
};
const reg_wide_bits_name_map = [8]*const [2]u8{
    "ax",
    "cx",
    "dx",
    "bx",
    "sp",
    "bp",
    "si",
    "di",
};
const reg_mem_effective_calc_map = [8][]const u8{
    "bx + si",
    "bx + di",
    "bp + si",
    "bp + di",
    "si",
    "di",
    "bp",
    "bx",
};
const MOV_INSTRUCTION = 0b00100010;
const IMM_INSTRUCTION = 0b00001011;

pub fn main(init: std.process.Init) !void {
    const io = init.io;
    const arena = init.arena;
    const arena_alloc = arena.allocator();
    const listing_0039_paths = [_][]const u8{ "..", "computer_enhance", "perfaware", "part1", "listing_0039_more_movs" };
    const listing_0039_path = try std.fs.path.join(arena_alloc, &listing_0039_paths);
    const content_listing_0039: []u8 = try Io.Dir.cwd().readFileAlloc(io, listing_0039_path, arena_alloc, .unlimited);
    const max_chars_per_instruction = "mov bx, [bx + si + 65535]".len;
    // could maybe less size since above instruction actually takes 4 bytes but also maybe some combination
    // could make it buffer overflow?
    const disassembly_buffer = try arena_alloc.alloc(u8, content_listing_0039.len / 2 * max_chars_per_instruction);

    const disassembly_slice = try disassemble_binary(disassembly_buffer, content_listing_0039);
    std.debug.print("{s}\n", .{disassembly_slice});

    const listing_0039_disassembly_output_paths = [_][]const u8{ "testing_results", "listing_0039_more_movs_disassembly.asm" };
    const listing_0039_disassembly_output_path = try std.fs.path.join(arena_alloc, &listing_0039_disassembly_output_paths);
    try std.Io.Dir.cwd().createDirPath(io, "testing_results");
    const file_listing_0039_disassembly = try std.Io.Dir.cwd().createFile(io, listing_0039_disassembly_output_path, .{ .truncate = false });
    try file_listing_0039_disassembly.writeStreamingAll(io, disassembly_slice);

    const nasm_result = try std.process.run(arena_alloc, io, .{
        .argv = &.{ "nasm", listing_0039_disassembly_output_path, "-o", "/dev/stdout" },
    });

    try std.testing.expectEqualStrings(content_listing_0039, nasm_result.stdout);
}

fn disassemble_binary(disassembly_buffer: []u8, content_listing_0039: []u8) ![]u8 {
    var disassembly_buffer_writer = std.Io.Writer.fixed(disassembly_buffer);

    try disassembly_buffer_writer.print("; listing_0039_more_movs disassembly:\nbits 16", .{});
    var i: u32 = 0;
    while (i < content_listing_0039.len) {
        const instruction = content_listing_0039[i] >> 2;
        if (instruction == MOV_INSTRUCTION) {
            const direction_bit = (content_listing_0039[i] >> 1) & 0b00000001;
            const wide_bit = content_listing_0039[i] & 0b00000001;

            i += 1;
            const mod = content_listing_0039[i] >> 6;

            const reg_field = (content_listing_0039[i] >> 3) & 0b00000111;
            const reg_mem_field = content_listing_0039[i] & 0b00000111;

            const reg_field_name = if (wide_bit == 1) reg_wide_bits_name_map[reg_field] else reg_bits_name_map[reg_field];
            const reg_mem_field_name = if (wide_bit == 1) reg_wide_bits_name_map[reg_mem_field] else reg_bits_name_map[reg_mem_field];
            const reg_mem_effective_calc_type = reg_mem_effective_calc_map[reg_mem_field];

            var disp: u16 = undefined;

            // no displacement (Except when R/M=110, then 16 bit displacement)
            if (mod == 0b000000) {
                // NOTE: skipping direct addresss, only relevant in 0040
                std.debug.assert(reg_mem_field != 0b110);
                if (direction_bit == 1) {
                    // e.g.: mov cx, [bx + si]
                    try disassembly_buffer_writer.print("\nmov {s}, [{s}]", .{ reg_field_name, reg_mem_effective_calc_type });
                } else {
                    // e.g.: mov [bx + si], cx
                    try disassembly_buffer_writer.print("\nmov [{s}], {s}", .{ reg_mem_effective_calc_type, reg_field_name });
                }
            }
            // 8 bit displacement
            else if (mod == 0b000001) {
                i += 1;
                disp = content_listing_0039[i];

                if (direction_bit == 1) {
                    try disassembly_buffer_writer.print("\nmov {s}, [{s} + {d}]", .{ reg_field_name, reg_mem_effective_calc_type, disp });
                } else {
                    try disassembly_buffer_writer.print("\nmov [{s} + {d}], {s}", .{ reg_mem_effective_calc_type, disp, reg_field_name });
                }
            }
            // 16 bit displacement
            else if (mod == 0b000010) {
                i += 1;
                const disp_lo = content_listing_0039[i];
                i += 1;
                const disp_high: u16 = content_listing_0039[i];
                disp = (disp_high << 8) | disp_lo;

                if (direction_bit == 1) {
                    try disassembly_buffer_writer.print("\nmov {s}, [{s} + {d}]", .{ reg_field_name, reg_mem_effective_calc_type, disp });
                } else {
                    try disassembly_buffer_writer.print("\nmov [{s} + {d}], {s}", .{ reg_mem_effective_calc_type, disp, reg_field_name });
                }
            }
            // Register mode (no displacement)
            else {
                if (direction_bit == 1) {
                    try disassembly_buffer_writer.print("\nmov {s}, {s}", .{ reg_field_name, reg_mem_field_name });
                } else {
                    try disassembly_buffer_writer.print("\nmov {s}, {s}", .{ reg_mem_field_name, reg_field_name });
                }
            }

            i += 1;
        } else if ((instruction >> 2) == IMM_INSTRUCTION) {
            const wide_bit = (content_listing_0039[i] >> 3) & 0b00000001;
            const reg_field = content_listing_0039[i] & 0b00000111;
            // const reg_field_name = if (wide_bit == 1) reg_wide_bits_name_map[reg_field] else reg_bits_name_map[reg_field];
            i += 1;
            if (wide_bit == 1) {
                const reg_field_name = reg_wide_bits_name_map[reg_field];
                const data_low = content_listing_0039[i];
                i += 1;
                const data_high: u16 = content_listing_0039[i];
                const data = (data_high << 8) | data_low;
                try disassembly_buffer_writer.print("\nmov {s}, {d}", .{ reg_field_name, data });
            } else {
                const reg_field_name = reg_bits_name_map[reg_field];
                const data = content_listing_0039[i];
                try disassembly_buffer_writer.print("\nmov {s}, {d}", .{ reg_field_name, data });
            }
            i += 1;
        }
    }
    return disassembly_buffer[0..disassembly_buffer_writer.end];
}

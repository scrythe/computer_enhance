const std = @import("std");
const Io = std.Io;

const reg_bits_name_map = [8]*const [2]u8{ "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
const reg_wide_bits_name_map = [8]*const [2]u8{ "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };

pub fn main(init: std.process.Init) !void {
    const io = init.io;
    const arena = init.arena;
    const arena_alloc = arena.allocator();
    const listing_0038_paths = [_][]const u8{ "computer_enhance", "perfaware", "part1", "listing_0038_many_register_mov" };
    const listing_0038_path = try std.fs.path.join(arena_alloc, &listing_0038_paths);
    const content_listing_0038 = try Io.Dir.cwd().readFileAlloc(io, listing_0038_path, arena_alloc, .unlimited);

    const n_instructions = content_listing_0038.len / 2;
    const size_disassembly_mov_intructions = 55 + n_instructions * 11;

    const disassembly_instructions_buffer = try arena_alloc.alloc(u8, size_disassembly_mov_intructions);
    disassembly_mov_intructions(content_listing_0038, disassembly_instructions_buffer);
    std.debug.print("{s}\n", .{disassembly_instructions_buffer});

    const listing_0038_disassembly_output_paths = [_][]const u8{ "testing_results", "listing_0038_single_register_mov_disassembly.asm" };
    const listing_0038_disassembly_output_path = try std.fs.path.join(arena_alloc, &listing_0038_disassembly_output_paths);
    try std.Io.Dir.cwd().createDirPath(io, "testing_results");
    const file_listing_0038_disassembly = try std.Io.Dir.cwd().createFile(io, listing_0038_disassembly_output_path, .{ .truncate = false });
    try file_listing_0038_disassembly.writeStreamingAll(io, disassembly_instructions_buffer);

    const nasm_result = try std.process.run(arena_alloc, io, .{
        .argv = &.{ "nasm", listing_0038_disassembly_output_path, "-o", "/dev/stdout" },
    });

    try std.testing.expectEqualStrings(content_listing_0038, nasm_result.stdout);
}

fn disassembly_mov_intructions(content_listing_0038: []const u8, disassembly_instructions_buffer: []u8) void {
    const disassembly_content_start = "; listing_0037_single_register_mov disassembly:\nbits 16";
    @memcpy(disassembly_instructions_buffer[0..disassembly_content_start.len], disassembly_content_start);
    var current = disassembly_content_start.len;

    const n_instructions = content_listing_0038.len / 2;
    for (0..n_instructions) |n_instrunction| {
        @memcpy(disassembly_instructions_buffer[current .. current + 11], "\nmov aa, aa");

        const nth_byte = n_instrunction * 2;

        const direction_bit = (content_listing_0038[nth_byte] >> 1) & 0b00000001;

        const wide_bit = content_listing_0038[nth_byte] & 0b00000001;

        const reg_field = (content_listing_0038[nth_byte + 1] >> 3) & 0b00000111;

        const reg_mem_field = content_listing_0038[nth_byte + 1] & 0b00000111;

        const reg_field_name = if (wide_bit == 1) reg_wide_bits_name_map[reg_field] else reg_bits_name_map[reg_field];
        const reg_mem_field_name = if (wide_bit == 1) reg_wide_bits_name_map[reg_mem_field] else reg_bits_name_map[reg_mem_field];

        if (direction_bit == 1) {
            @memcpy(disassembly_instructions_buffer[current + 5 .. current + 7], reg_field_name);
            @memcpy(disassembly_instructions_buffer[current + 9 .. current + 11], reg_mem_field_name);
        } else {
            @memcpy(disassembly_instructions_buffer[current + 5 .. current + 7], reg_mem_field_name);
            @memcpy(disassembly_instructions_buffer[current + 9 .. current + 11], reg_field_name);
        }

        current += 11;
    }
}

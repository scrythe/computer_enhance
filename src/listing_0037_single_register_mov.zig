const std = @import("std");
const Io = std.Io;

const reg_bits_name_map = [8]*const [2]u8{ "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
const reg_wide_bits_name_map = [8]*const [2]u8{ "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };

pub fn main(init: std.process.Init) !void {
    const io = init.io;
    const arena = init.arena;
    const arena_alloc = arena.allocator();
    const listing_0037_paths = [_][]const u8{ "computer_enhance", "perfaware", "part1", "listing_0037_single_register_mov" };
    const listing_0037_path = try std.fs.path.join(arena_alloc, &listing_0037_paths);
    const content_listing_0037 = try Io.Dir.cwd().readFileAlloc(io, listing_0037_path, arena_alloc, .unlimited);

    var output_buffer: [66]u8 = undefined;

    try disassembly_0037(content_listing_0037, &output_buffer);

    std.debug.print("{s}\n", .{output_buffer});
}

// writes to output_buffer with 66 bytes
fn disassembly_0037(content_listing_0037: []u8, output_buffer: []u8) !void {
    // last 6 bits of first byte
    // should be 100010 becaue of mov
    // const mov_instruction = content_listing_0037[0] >> 2;

    // 2 bit of first byte
    const direction_bit = (content_listing_0037[0] >> 1) & 0b00000001;

    // 1 bit of first byte
    const wide_bit = content_listing_0037[0] & 0b00000001;

    // last 2 bits of second byte
    // should be 11 because register to register
    // const mod = content_listing_0037[1] >> 6;

    // first 3 bits from second register
    const reg_field = (content_listing_0037[1] >> 3) & 0b00000111;

    const reg_mem_field = content_listing_0037[1] & 0b00000111;

    const reg_field_name = if (wide_bit == 1) reg_wide_bits_name_map[reg_field] else reg_bits_name_map[reg_field];
    const reg_mem_field_name = if (wide_bit == 1) reg_wide_bits_name_map[reg_mem_field] else reg_bits_name_map[reg_mem_field];

    // aa is temp register and will be replaced below in if else statement
    const disassembly_content = "; listing_0037_single_register_mov disassembly:\nbits 16\nmov aa, aa";
    @memcpy(output_buffer[0..disassembly_content.len], disassembly_content);

    if (direction_bit == 1) {
        @memcpy(output_buffer[disassembly_content.len - 6 .. disassembly_content.len - 4], reg_field_name);
        @memcpy(output_buffer[disassembly_content.len - 2 .. disassembly_content.len], reg_mem_field_name);
    } else {
        @memcpy(output_buffer[disassembly_content.len - 6 .. disassembly_content.len - 4], reg_mem_field_name);
        @memcpy(output_buffer[disassembly_content.len - 2 .. disassembly_content.len], reg_field_name);
    }
}

const testing = std.testing;
test "test disassembly_0037" {
    const io = testing.io;
    var arena = std.heap.ArenaAllocator.init(testing.allocator);
    defer arena.deinit();
    const arena_alloc = arena.allocator();
    const listing_0037_paths = [_][]const u8{ "computer_enhance", "perfaware", "part1", "listing_0037_single_register_mov" };
    const listing_0037_path = try std.fs.path.join(arena_alloc, &listing_0037_paths);
    const content_listing_0037 = try Io.Dir.cwd().readFileAlloc(io, listing_0037_path, arena_alloc, .unlimited);

    const listing_0037_disassembly_output_paths = [_][]const u8{ "testing_results", "listing_0037_single_register_mov_disassembly.asm" };
    const listing_0037_disassembly_output_path = try std.fs.path.join(arena_alloc, &listing_0037_disassembly_output_paths);
    try std.Io.Dir.cwd().createDirPath(io, "testing_results");
    const file_listing_0037 = try std.Io.Dir.cwd().createFile(io, listing_0037_disassembly_output_path, .{ .truncate = false });

    var output_buffer: [66]u8 = undefined;
    try disassembly_0037(content_listing_0037, &output_buffer);

    try file_listing_0037.writeStreamingAll(io, &output_buffer);

    const nasm_result = try std.process.run(arena_alloc, io, .{
        .argv = &.{ "nasm", listing_0037_disassembly_output_path, "-o", "/dev/stdout" },
    });

    try testing.expectEqualStrings(content_listing_0037, nasm_result.stdout);
}

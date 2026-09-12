const std = @import("std");
const Io = std.Io;
const Allocator = std.mem.Allocator;

const sim86 = @import("sim86");
const Instruction = sim86.Instruction;

extern fn Sim86_Decode8086Instruction(SourceSize: u32, Source: [*]u8, Dest: *sim86.Instruction) void;

pub fn main(init: std.process.Init) !void {
    const io = init.io;
    const arena = init.arena;
    const arena_alloc = arena.allocator();
    const listing_paths = [_][]const u8{ "computer_enhance", "perfaware", "part1", "listing_0043_immediate_movs" };
    const listing_path = try std.fs.path.join(arena_alloc, &listing_paths);
    const content_listing: []u8 = try Io.Dir.cwd().readFileAlloc(io, listing_path, arena_alloc, .unlimited);

    const listing_expected_output_paths = [_][]const u8{ "computer_enhance", "perfaware", "part1", "listing_0043_immediate_movs.txt" };
    const listing_expected_output_path = try std.fs.path.join(arena_alloc, &listing_expected_output_paths);
    const listing_expected_output: []u8 = try Io.Dir.cwd().readFileAlloc(io, listing_expected_output_path, arena_alloc, .unlimited);

    const expected_output_buffer = try arena_alloc.alloc(u8, listing_expected_output.len * 2);
    var expected_output_writer = std.Io.Writer.fixed(expected_output_buffer);

    try std.testing.expectEqual(sim86.getVersion(), 4);

    var registers: [8]u16 = undefined;
    @memset(&registers, 0);

    try expected_output_writer.print("--- test\\listing_0043_immediate_movs execution ---\r\n", .{});

    var current_pos: u32 = 0;
    while (current_pos < content_listing.len) {
        const decoded = try sim86.decode8086Instruction(content_listing[current_pos..]);
        var reg = decoded.Operands[0].data.Register;
        const reg_name = sim86.registerNameFromOperand(&reg);
        const prev_data = registers[reg.Index - 1];
        const data: u16 = @intCast(decoded.Operands[0].data.Immediate.Value);
        registers[reg.Index - 1] = data;
        try expected_output_writer.print("mov {s}, {d} ; {s}:0x{x}->0x{x} \r\n", .{ reg_name, data, reg_name, prev_data, data });

        current_pos += decoded.Size;
    }

    try expected_output_writer.print("\r\nFinal registers:\r\n", .{});
    for (registers, 0..) |register, reg_i| {
        var reg =
            sim86.RegisterAccess{
                .Index = @intCast(reg_i + 1),
                .Offset = 0,
                .Count = 2,
            };
        const reg_name = sim86.registerNameFromOperand(&reg);
        try expected_output_writer.print("      {s}: 0x{x:0>4} ({d})\r\n", .{ reg_name, register, register });
    }
    try expected_output_writer.print("\r\n", .{});

    const expected_output = expected_output_buffer[0..expected_output_writer.end];
    std.debug.print("{s}", .{expected_output});

    try check_output(arena_alloc, io, expected_output);
}

fn check_output(arena_alloc: Allocator, io: Io, listing_output: []const u8) !void {
    const listing_expected_output_paths = [_][]const u8{ "computer_enhance", "perfaware", "part1", "listing_0043_immediate_movs.txt" };
    const listing_expected_output_path = try std.fs.path.join(arena_alloc, &listing_expected_output_paths);
    const listing_expected_output: []u8 = try Io.Dir.cwd().readFileAlloc(io, listing_expected_output_path, arena_alloc, .unlimited);
    try std.testing.expectEqualStrings(listing_expected_output, listing_output);
}

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
    const listing_paths = [_][]const u8{ "..", "computer_enhance", "perfaware", "part1", "listing_0045_challenge_register_movs" };
    const listing_path = try std.fs.path.join(arena_alloc, &listing_paths);
    const listing_content: []u8 = try Io.Dir.cwd().readFileAlloc(io, listing_path, arena_alloc, .unlimited);

    const listing_expected_output_paths = [_][]const u8{ "..", "computer_enhance", "perfaware", "part1", "listing_0045_challenge_register_movs.txt" };
    const listing_expected_output_path = try std.fs.path.join(arena_alloc, &listing_expected_output_paths);
    const listing_expected_output: []u8 = try Io.Dir.cwd().readFileAlloc(io, listing_expected_output_path, arena_alloc, .unlimited);

    const listing_output_buffer = try arena_alloc.alloc(u8, listing_expected_output.len * 2);
    var listing_output_writer = std.Io.Writer.fixed(listing_output_buffer);
    try listing_output_writer.print("--- test\\listing_0045_challenge_register_movs execution ---\r\n", .{});

    errdefer std.debug.print("{s}", .{listing_output_buffer});

    var temp_print_buffer: [1024]u8 = undefined;
    var temp_print_writer = std.Io.Writer.fixed(&temp_print_buffer);

    try std.testing.expectEqual(sim86.getVersion(), 4);

    var registers: [14]u16 = undefined;
    @memset(&registers, 0);

    var current_pos: u32 = 0;
    while (current_pos < listing_content.len) {
        const decoded = try sim86.decode8086Instruction(listing_content[current_pos..]);
        var reg = decoded.Operands[0].data.Register;
        const reg_name = sim86.registerNameFromOperand(&reg);
        var reg_word = sim86.RegisterAccess{ .Index = reg.Index, .Offset = 0, .Count = 2 };
        const reg_word_name = sim86.registerNameFromOperand(&reg_word);
        const prev_data = registers[reg.Index - 1];
        const second_operand = decoded.Operands[1];
        var second_operand_data: u16 = undefined;
        var second_operand_name: []const u8 = undefined;
        if (second_operand.Type == .OperandRegister) {
            var second_operand_register = second_operand.data.Register;
            second_operand_data = registers[second_operand_register.Index - 1];
            if (second_operand_register.Count == 2) {
                second_operand_data = registers[second_operand_register.Index - 1];
            } else {
                if (second_operand_register.Offset == 1) {
                    second_operand_data = registers[second_operand_register.Index - 1] >> 8;
                } else {
                    second_operand_data = registers[second_operand_register.Index - 1] & 0x00FF;
                }
            }
            second_operand_name = sim86.registerNameFromOperand(&second_operand_register);
        } else {
            second_operand_data = @intCast(second_operand.data.Immediate.Value);
            const start = temp_print_writer.end;
            try temp_print_writer.printInt(second_operand_data, 10, .lower, .{});
            second_operand_name = temp_print_buffer[start..temp_print_writer.end];
        }
        if (reg.Count == 2) {
            registers[reg.Index - 1] = second_operand_data;
        } else {
            registers[reg.Index - 1] =
                if (reg.Offset == 1)
                    (registers[reg.Index - 1] & 0x00FF) | (second_operand_data << 8)
                else
                    (registers[reg.Index - 1] & 0xFF00) | second_operand_data;
        }
        const register_new_data = registers[reg.Index - 1];
        try listing_output_writer.print("mov {s}, {s} ; {s}:0x{x}->0x{x} \r\n", .{ reg_name, second_operand_name, reg_word_name, prev_data, register_new_data });

        current_pos += decoded.Size;
    }
    try listing_output_writer.print("\r\nFinal registers:\r\n", .{});
    for (registers, 0..) |register, reg_i| {
        if (register != 0) {
            var reg =
                sim86.RegisterAccess{
                    .Index = @intCast(reg_i + 1),
                    .Offset = 0,
                    .Count = 2,
                };
            const reg_name = sim86.registerNameFromOperand(&reg);
            try listing_output_writer.print("      {s}: 0x{x:0>4} ({d})\r\n", .{ reg_name, register, register });
        }
    }
    try listing_output_writer.print("\r\n", .{});

    const listing_output = listing_output_buffer[0..listing_output_writer.end];
    std.debug.print("{s}", .{listing_output});

    try std.testing.expectEqualStrings(listing_expected_output, listing_output);
}

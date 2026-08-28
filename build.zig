const std = @import("std");

pub fn build(b: *std.Build) !void {
    const allocator = b.allocator;
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const debug_build = b.option(bool, "debug", "debug build");

    const run_step = b.step("run", "Run the compiler");
    const test_step = b.step("test", "Run tests");

    const filename = b.option([]const u8, "file", "File to execute") orelse {
        std.debug.print("Missing File, use -Dfile=<filename>\n", .{});
        return;
    };
    const file = try allocator.alloc(u8, filename.len + 4);
    @memcpy(file[0..filename.len], filename);
    @memcpy(file[filename.len..], ".zig");
    const file_path = try std.fs.path.join(allocator, &.{ "src", file });

    const exe = b.addExecutable(.{
        .name = filename,
        .use_llvm = debug_build,
        .root_module = b.createModule(.{
            .root_source_file = b.path(file_path),
            .target = target,
            .optimize = optimize,
            .link_libc = debug_build,
        }),
    });
    exe.pie = debug_build;

    const exe_tests = b.addTest(.{ .root_module = exe.root_module });

    b.installArtifact(exe);
    // b.installArtifact(exe_tests);

    const run_cmd = b.addRunArtifact(exe);
    run_step.dependOn(&run_cmd.step);

    const install_on_run_cmd = b.addInstallArtifact(exe, .{});
    run_step.dependOn(&install_on_run_cmd.step);

    const test_cmd = b.addRunArtifact(exe_tests);
    const install_tests_cmd = b.addInstallArtifact(exe_tests, .{});
    test_step.dependOn(&test_cmd.step);
    test_step.dependOn(&install_tests_cmd.step);
}

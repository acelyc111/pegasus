// Autogenerated by Thrift Compiler (0.11.0)
// DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING

package main

import (
        "context"
        "flag"
        "fmt"
        "math"
        "net"
        "net/url"
        "os"
        "strconv"
        "strings"
        "github.com/apache/thrift/lib/go/thrift"
	"github.com/apache/incubator-pegasus/go-client/idl/base"
        "github.com/apache/incubator-pegasus/go-client/idl/rrdb"
)

var _ = base.GoUnusedProtection__

func Usage() {
  fmt.Fprintln(os.Stderr, "Usage of ", os.Args[0], " [-h host:port] [-u url] [-f[ramed]] function [arg1 [arg2...]]:")
  flag.PrintDefaults()
  fmt.Fprintln(os.Stderr, "\nFunctions:")
  fmt.Fprintln(os.Stderr, "  update_response put(update_request update)")
  fmt.Fprintln(os.Stderr, "  update_response multi_put(multi_put_request request)")
  fmt.Fprintln(os.Stderr, "  update_response remove(blob key)")
  fmt.Fprintln(os.Stderr, "  multi_remove_response multi_remove(multi_remove_request request)")
  fmt.Fprintln(os.Stderr, "  incr_response incr(incr_request request)")
  fmt.Fprintln(os.Stderr, "  check_and_set_response check_and_set(check_and_set_request request)")
  fmt.Fprintln(os.Stderr, "  check_and_mutate_response check_and_mutate(check_and_mutate_request request)")
  fmt.Fprintln(os.Stderr, "  read_response get(blob key)")
  fmt.Fprintln(os.Stderr, "  multi_get_response multi_get(multi_get_request request)")
  fmt.Fprintln(os.Stderr, "  batch_get_response batch_get(batch_get_request request)")
  fmt.Fprintln(os.Stderr, "  count_response sortkey_count(blob hash_key)")
  fmt.Fprintln(os.Stderr, "  ttl_response ttl(blob key)")
  fmt.Fprintln(os.Stderr, "  scan_response get_scanner(get_scanner_request request)")
  fmt.Fprintln(os.Stderr, "  scan_response scan(scan_request request)")
  fmt.Fprintln(os.Stderr, "  void clear_scanner(i64 context_id)")
  fmt.Fprintln(os.Stderr)
  os.Exit(0)
}

func main() {
  flag.Usage = Usage
  var host string
  var port int
  var protocol string
  var urlString string
  var framed bool
  var useHttp bool
  var parsedUrl *url.URL
  var trans thrift.TTransport
  _ = strconv.Atoi
  _ = math.Abs
  flag.Usage = Usage
  flag.StringVar(&host, "h", "localhost", "Specify host and port")
  flag.IntVar(&port, "p", 9090, "Specify port")
  flag.StringVar(&protocol, "P", "binary", "Specify the protocol (binary, compact, simplejson, json)")
  flag.StringVar(&urlString, "u", "", "Specify the url")
  flag.BoolVar(&framed, "framed", false, "Use framed transport")
  flag.BoolVar(&useHttp, "http", false, "Use http")
  flag.Parse()
  
  if len(urlString) > 0 {
    var err error
    parsedUrl, err = url.Parse(urlString)
    if err != nil {
      fmt.Fprintln(os.Stderr, "Error parsing URL: ", err)
      flag.Usage()
    }
    host = parsedUrl.Host
    useHttp = len(parsedUrl.Scheme) <= 0 || parsedUrl.Scheme == "http"
  } else if useHttp {
    _, err := url.Parse(fmt.Sprint("http://", host, ":", port))
    if err != nil {
      fmt.Fprintln(os.Stderr, "Error parsing URL: ", err)
      flag.Usage()
    }
  }
  
  cmd := flag.Arg(0)
  var err error
  if useHttp {
    trans, err = thrift.NewTHttpClient(parsedUrl.String())
  } else {
    portStr := fmt.Sprint(port)
    if strings.Contains(host, ":") {
           host, portStr, err = net.SplitHostPort(host)
           if err != nil {
                   fmt.Fprintln(os.Stderr, "error with host:", err)
                   os.Exit(1)
           }
    }
    trans, err = thrift.NewTSocket(net.JoinHostPort(host, portStr))
    if err != nil {
      fmt.Fprintln(os.Stderr, "error resolving address:", err)
      os.Exit(1)
    }
    if framed {
      trans = thrift.NewTFramedTransport(trans)
    }
  }
  if err != nil {
    fmt.Fprintln(os.Stderr, "Error creating transport", err)
    os.Exit(1)
  }
  defer trans.Close()
  var protocolFactory thrift.TProtocolFactory
  switch protocol {
  case "compact":
    protocolFactory = thrift.NewTCompactProtocolFactory()
    break
  case "simplejson":
    protocolFactory = thrift.NewTSimpleJSONProtocolFactory()
    break
  case "json":
    protocolFactory = thrift.NewTJSONProtocolFactory()
    break
  case "binary", "":
    protocolFactory = thrift.NewTBinaryProtocolFactoryDefault()
    break
  default:
    fmt.Fprintln(os.Stderr, "Invalid protocol specified: ", protocol)
    Usage()
    os.Exit(1)
  }
  iprot := protocolFactory.GetProtocol(trans)
  oprot := protocolFactory.GetProtocol(trans)
  client := rrdb.NewRrdbClient(thrift.NewTStandardClient(iprot, oprot))
  if err := trans.Open(); err != nil {
    fmt.Fprintln(os.Stderr, "Error opening socket to ", host, ":", port, " ", err)
    os.Exit(1)
  }
  
  switch cmd {
  case "put":
    if flag.NArg() - 1 != 1 {
      fmt.Fprintln(os.Stderr, "Put requires 1 args")
      flag.Usage()
    }
    arg40 := flag.Arg(1)
    mbTrans41 := thrift.NewTMemoryBufferLen(len(arg40))
    defer mbTrans41.Close()
    _, err42 := mbTrans41.WriteString(arg40)
    if err42 != nil {
      Usage()
      return
    }
    factory43 := thrift.NewTSimpleJSONProtocolFactory()
    jsProt44 := factory43.GetProtocol(mbTrans41)
    argvalue0 := rrdb.NewUpdateRequest()
    err45 := argvalue0.Read(jsProt44)
    if err45 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    fmt.Print(client.Put(context.Background(), value0))
    fmt.Print("\n")
    break
  case "multi_put":
    if flag.NArg() - 1 != 1 {
      fmt.Fprintln(os.Stderr, "MultiPut requires 1 args")
      flag.Usage()
    }
    arg46 := flag.Arg(1)
    mbTrans47 := thrift.NewTMemoryBufferLen(len(arg46))
    defer mbTrans47.Close()
    _, err48 := mbTrans47.WriteString(arg46)
    if err48 != nil {
      Usage()
      return
    }
    factory49 := thrift.NewTSimpleJSONProtocolFactory()
    jsProt50 := factory49.GetProtocol(mbTrans47)
    argvalue0 := rrdb.NewMultiPutRequest()
    err51 := argvalue0.Read(jsProt50)
    if err51 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    fmt.Print(client.MultiPut(context.Background(), value0))
    fmt.Print("\n")
    break
  case "remove":
    if flag.NArg() - 1 != 1 {
      fmt.Fprintln(os.Stderr, "Remove requires 1 args")
      flag.Usage()
    }
    arg52 := flag.Arg(1)
    mbTrans53 := thrift.NewTMemoryBufferLen(len(arg52))
    defer mbTrans53.Close()
    _, err54 := mbTrans53.WriteString(arg52)
    if err54 != nil {
      Usage()
      return
    }
    factory55 := thrift.NewTSimpleJSONProtocolFactory()
    jsProt56 := factory55.GetProtocol(mbTrans53)
    argvalue0 := base.NewBlob()
    err57 := argvalue0.Read(jsProt56)
    if err57 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    fmt.Print(client.Remove(context.Background(), value0))
    fmt.Print("\n")
    break
  case "multi_remove":
    if flag.NArg() - 1 != 1 {
      fmt.Fprintln(os.Stderr, "MultiRemove requires 1 args")
      flag.Usage()
    }
    arg58 := flag.Arg(1)
    mbTrans59 := thrift.NewTMemoryBufferLen(len(arg58))
    defer mbTrans59.Close()
    _, err60 := mbTrans59.WriteString(arg58)
    if err60 != nil {
      Usage()
      return
    }
    factory61 := thrift.NewTSimpleJSONProtocolFactory()
    jsProt62 := factory61.GetProtocol(mbTrans59)
    argvalue0 := rrdb.NewMultiRemoveRequest()
    err63 := argvalue0.Read(jsProt62)
    if err63 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    fmt.Print(client.MultiRemove(context.Background(), value0))
    fmt.Print("\n")
    break
  case "incr":
    if flag.NArg() - 1 != 1 {
      fmt.Fprintln(os.Stderr, "Incr requires 1 args")
      flag.Usage()
    }
    arg64 := flag.Arg(1)
    mbTrans65 := thrift.NewTMemoryBufferLen(len(arg64))
    defer mbTrans65.Close()
    _, err66 := mbTrans65.WriteString(arg64)
    if err66 != nil {
      Usage()
      return
    }
    factory67 := thrift.NewTSimpleJSONProtocolFactory()
    jsProt68 := factory67.GetProtocol(mbTrans65)
    argvalue0 := rrdb.NewIncrRequest()
    err69 := argvalue0.Read(jsProt68)
    if err69 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    fmt.Print(client.Incr(context.Background(), value0))
    fmt.Print("\n")
    break
  case "check_and_set":
    if flag.NArg() - 1 != 1 {
      fmt.Fprintln(os.Stderr, "CheckAndSet requires 1 args")
      flag.Usage()
    }
    arg70 := flag.Arg(1)
    mbTrans71 := thrift.NewTMemoryBufferLen(len(arg70))
    defer mbTrans71.Close()
    _, err72 := mbTrans71.WriteString(arg70)
    if err72 != nil {
      Usage()
      return
    }
    factory73 := thrift.NewTSimpleJSONProtocolFactory()
    jsProt74 := factory73.GetProtocol(mbTrans71)
    argvalue0 := rrdb.NewCheckAndSetRequest()
    err75 := argvalue0.Read(jsProt74)
    if err75 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    fmt.Print(client.CheckAndSet(context.Background(), value0))
    fmt.Print("\n")
    break
  case "check_and_mutate":
    if flag.NArg() - 1 != 1 {
      fmt.Fprintln(os.Stderr, "CheckAndMutate requires 1 args")
      flag.Usage()
    }
    arg76 := flag.Arg(1)
    mbTrans77 := thrift.NewTMemoryBufferLen(len(arg76))
    defer mbTrans77.Close()
    _, err78 := mbTrans77.WriteString(arg76)
    if err78 != nil {
      Usage()
      return
    }
    factory79 := thrift.NewTSimpleJSONProtocolFactory()
    jsProt80 := factory79.GetProtocol(mbTrans77)
    argvalue0 := rrdb.NewCheckAndMutateRequest()
    err81 := argvalue0.Read(jsProt80)
    if err81 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    fmt.Print(client.CheckAndMutate(context.Background(), value0))
    fmt.Print("\n")
    break
  case "get":
    if flag.NArg() - 1 != 1 {
      fmt.Fprintln(os.Stderr, "Get requires 1 args")
      flag.Usage()
    }
    arg82 := flag.Arg(1)
    mbTrans83 := thrift.NewTMemoryBufferLen(len(arg82))
    defer mbTrans83.Close()
    _, err84 := mbTrans83.WriteString(arg82)
    if err84 != nil {
      Usage()
      return
    }
    factory85 := thrift.NewTSimpleJSONProtocolFactory()
    jsProt86 := factory85.GetProtocol(mbTrans83)
    argvalue0 := base.NewBlob()
    err87 := argvalue0.Read(jsProt86)
    if err87 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    fmt.Print(client.Get(context.Background(), value0))
    fmt.Print("\n")
    break
  case "multi_get":
    if flag.NArg() - 1 != 1 {
      fmt.Fprintln(os.Stderr, "MultiGet requires 1 args")
      flag.Usage()
    }
    arg88 := flag.Arg(1)
    mbTrans89 := thrift.NewTMemoryBufferLen(len(arg88))
    defer mbTrans89.Close()
    _, err90 := mbTrans89.WriteString(arg88)
    if err90 != nil {
      Usage()
      return
    }
    factory91 := thrift.NewTSimpleJSONProtocolFactory()
    jsProt92 := factory91.GetProtocol(mbTrans89)
    argvalue0 := rrdb.NewMultiGetRequest()
    err93 := argvalue0.Read(jsProt92)
    if err93 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    fmt.Print(client.MultiGet(context.Background(), value0))
    fmt.Print("\n")
    break
  case "batch_get":
    if flag.NArg() - 1 != 1 {
      fmt.Fprintln(os.Stderr, "BatchGet requires 1 args")
      flag.Usage()
    }
    arg94 := flag.Arg(1)
    mbTrans95 := thrift.NewTMemoryBufferLen(len(arg94))
    defer mbTrans95.Close()
    _, err96 := mbTrans95.WriteString(arg94)
    if err96 != nil {
      Usage()
      return
    }
    factory97 := thrift.NewTSimpleJSONProtocolFactory()
    jsProt98 := factory97.GetProtocol(mbTrans95)
    argvalue0 := rrdb.NewBatchGetRequest()
    err99 := argvalue0.Read(jsProt98)
    if err99 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    fmt.Print(client.BatchGet(context.Background(), value0))
    fmt.Print("\n")
    break
  case "sortkey_count":
    if flag.NArg() - 1 != 1 {
      fmt.Fprintln(os.Stderr, "SortkeyCount requires 1 args")
      flag.Usage()
    }
    arg100 := flag.Arg(1)
    mbTrans101 := thrift.NewTMemoryBufferLen(len(arg100))
    defer mbTrans101.Close()
    _, err102 := mbTrans101.WriteString(arg100)
    if err102 != nil {
      Usage()
      return
    }
    factory103 := thrift.NewTSimpleJSONProtocolFactory()
    jsProt104 := factory103.GetProtocol(mbTrans101)
    argvalue0 := base.NewBlob()
    err105 := argvalue0.Read(jsProt104)
    if err105 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    fmt.Print(client.SortkeyCount(context.Background(), value0))
    fmt.Print("\n")
    break
  case "ttl":
    if flag.NArg() - 1 != 1 {
      fmt.Fprintln(os.Stderr, "TTL requires 1 args")
      flag.Usage()
    }
    arg106 := flag.Arg(1)
    mbTrans107 := thrift.NewTMemoryBufferLen(len(arg106))
    defer mbTrans107.Close()
    _, err108 := mbTrans107.WriteString(arg106)
    if err108 != nil {
      Usage()
      return
    }
    factory109 := thrift.NewTSimpleJSONProtocolFactory()
    jsProt110 := factory109.GetProtocol(mbTrans107)
    argvalue0 := base.NewBlob()
    err111 := argvalue0.Read(jsProt110)
    if err111 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    fmt.Print(client.TTL(context.Background(), value0))
    fmt.Print("\n")
    break
  case "get_scanner":
    if flag.NArg() - 1 != 1 {
      fmt.Fprintln(os.Stderr, "GetScanner requires 1 args")
      flag.Usage()
    }
    arg112 := flag.Arg(1)
    mbTrans113 := thrift.NewTMemoryBufferLen(len(arg112))
    defer mbTrans113.Close()
    _, err114 := mbTrans113.WriteString(arg112)
    if err114 != nil {
      Usage()
      return
    }
    factory115 := thrift.NewTSimpleJSONProtocolFactory()
    jsProt116 := factory115.GetProtocol(mbTrans113)
    argvalue0 := rrdb.NewGetScannerRequest()
    err117 := argvalue0.Read(jsProt116)
    if err117 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    fmt.Print(client.GetScanner(context.Background(), value0))
    fmt.Print("\n")
    break
  case "scan":
    if flag.NArg() - 1 != 1 {
      fmt.Fprintln(os.Stderr, "Scan requires 1 args")
      flag.Usage()
    }
    arg118 := flag.Arg(1)
    mbTrans119 := thrift.NewTMemoryBufferLen(len(arg118))
    defer mbTrans119.Close()
    _, err120 := mbTrans119.WriteString(arg118)
    if err120 != nil {
      Usage()
      return
    }
    factory121 := thrift.NewTSimpleJSONProtocolFactory()
    jsProt122 := factory121.GetProtocol(mbTrans119)
    argvalue0 := rrdb.NewScanRequest()
    err123 := argvalue0.Read(jsProt122)
    if err123 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    fmt.Print(client.Scan(context.Background(), value0))
    fmt.Print("\n")
    break
  case "clear_scanner":
    if flag.NArg() - 1 != 1 {
      fmt.Fprintln(os.Stderr, "ClearScanner requires 1 args")
      flag.Usage()
    }
    argvalue0, err124 := (strconv.ParseInt(flag.Arg(1), 10, 64))
    if err124 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    fmt.Print(client.ClearScanner(context.Background(), value0))
    fmt.Print("\n")
    break
  case "":
    Usage()
    break
  default:
    fmt.Fprintln(os.Stderr, "Invalid function ", cmd)
  }
}